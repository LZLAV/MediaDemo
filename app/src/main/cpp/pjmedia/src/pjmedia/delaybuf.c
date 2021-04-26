/**
 * 已完成:
 *  学习过程：
 *
 */

#include <pjmedia/delaybuf.h>
#include <pjmedia/circbuf.h>
#include <pjmedia/errno.h>
#include <pjmedia/frame.h>
#include <pjmedia/wsola.h>
#include <pj/assert.h>
#include <pj/lock.h>
#include <pj/log.h>
#include <pj/math.h>
#include <pj/pool.h>


#if 0
#   define TRACE__(x) PJ_LOG(3,x)
#else
#   define TRACE__(x)
#endif

/* 延迟缓冲器的操作类型 */
enum OP
{
    OP_PUT,
    OP_GET
};

/*
 * 指定 delaybuf 重新计算有效延迟的时间（毫秒）
 */
#define RECALC_TIME	    2000

/*
 * 最大延迟的默认值（毫秒），当请求的最大延迟小于ptime（一帧长度）时使用此值
 */
#define DEFAULT_MAX_DELAY   400

/* 要添加到学习级别以获得额外稳定性的帧数。Number of frames to add to learnt level for additional stability
 */
#define SAFE_MARGIN	    0

/* 该结构描述内部延迟BUF设置和状态
 */
struct pjmedia_delay_buf
{
    /* 属性和配置 */
    char	     obj_name[PJ_MAX_OBJ_NAME];
    pj_lock_t	    *lock;		/**< 锁对象   */
    unsigned	     samples_per_frame; /**< 一帧中的样本数  */
    unsigned	     ptime;		/**< 帧时间，ms		     */
    unsigned	     channel_count;	/**< 通道数，毫秒     */
    pjmedia_circ_buf *circ_buf;		/**< 循环缓冲区，用于存储音频样本 */
    unsigned	     max_cnt;		/**< 要缓冲的最大样本数  */
    unsigned	     eff_cnt;		/**< 缓冲样本的有效计数，以保持延迟和稳定性之间的最佳平衡。这是根据突发级别计算的 */

    /* 学习变量 */
    unsigned	     level;		/**< 突发电平计数器     */
    enum OP	     last_op;		/**< 学习的最近一次操作(PUT 或 GET)*/
    int		     recalc_timer;	/**< 用于重新计算max_level 的计时器 */
    unsigned	     max_level;		/**< 当前最大脉冲电平	     */

    /* 漂移处理器*/
    pjmedia_wsola   *wsola;		/**< 漂移处理器   */
};


PJ_DEF(pj_status_t) pjmedia_delay_buf_create( pj_pool_t *pool,
					      const char *name,
					      unsigned clock_rate,
					      unsigned samples_per_frame,
					      unsigned channel_count,
					      unsigned max_delay,
					      unsigned options,
					      pjmedia_delay_buf **p_b)
{
    pjmedia_delay_buf *b;
    pj_status_t status;

    PJ_ASSERT_RETURN(pool && samples_per_frame && clock_rate && channel_count &&
		     p_b, PJ_EINVAL);

    if (!name) {
	name = "delaybuf";
    }

    b = PJ_POOL_ZALLOC_T(pool, pjmedia_delay_buf);

    pj_ansi_strncpy(b->obj_name, name, PJ_MAX_OBJ_NAME-1);

    b->samples_per_frame = samples_per_frame;
    b->channel_count = channel_count;
    b->ptime = samples_per_frame * 1000 / clock_rate / channel_count;
    if (max_delay < b->ptime)
	max_delay = PJ_MAX(DEFAULT_MAX_DELAY, b->ptime);

    b->max_cnt = samples_per_frame * max_delay / b->ptime;
    b->eff_cnt = b->max_cnt >> 1;
    b->recalc_timer = RECALC_TIME;

    /* 创建循环buf */
    status = pjmedia_circ_buf_create(pool, b->max_cnt, &b->circ_buf);
    if (status != PJ_SUCCESS)
	return status;

    if (!(options & PJMEDIA_DELAY_BUF_SIMPLE_FIFO)) {
        /* 创建 WSOLA */
        status = pjmedia_wsola_create(pool, clock_rate, samples_per_frame, 1,
				      PJMEDIA_WSOLA_NO_FADING, &b->wsola);
        if (status != PJ_SUCCESS)
	    return status;
        PJ_LOG(5, (b->obj_name, "Using delay buffer with WSOLA."));
    } else {
        PJ_LOG(5, (b->obj_name, "Using simple FIFO delay buffer."));
    }

    /* 最后，创建互斥量 */
    status = pj_lock_create_recursive_mutex(pool, b->obj_name, 
					    &b->lock);
    if (status != PJ_SUCCESS)
	return status;

    *p_b = b;

    TRACE__((b->obj_name,"Delay buffer created"));

    return PJ_SUCCESS;
}

PJ_DEF(pj_status_t) pjmedia_delay_buf_destroy(pjmedia_delay_buf *b)
{
    pj_status_t status = PJ_SUCCESS;

    PJ_ASSERT_RETURN(b, PJ_EINVAL);

    pj_lock_acquire(b->lock);

    if (b->wsola) {
        status = pjmedia_wsola_destroy(b->wsola);
        if (status == PJ_SUCCESS)
	    b->wsola = NULL;
    }

    pj_lock_release(b->lock);

    pj_lock_destroy(b->lock);
    b->lock = NULL;

    return status;
}

/* 此函数将从延迟缓冲区中删除样本。
 * 擦除样本的数量保证为 >= erase_cnt
 */
static void shrink_buffer(pjmedia_delay_buf *b, unsigned erase_cnt)
{
    pj_int16_t *buf1, *buf2;
    unsigned buf1len;
    unsigned buf2len;
    pj_status_t status;

    pj_assert(b && erase_cnt && pjmedia_circ_buf_get_len(b->circ_buf));

    pjmedia_circ_buf_get_read_regions(b->circ_buf, &buf1, &buf1len, 
				      &buf2, &buf2len);
    status = pjmedia_wsola_discard(b->wsola, buf1, buf1len, buf2, buf2len,
				   &erase_cnt);

    if ((status == PJ_SUCCESS) && (erase_cnt > 0)) {
	/* WSOLA discard将管理第一个缓冲区是否已满，除非erase_cnt大于第二个缓冲区长度。因此，只需设置循环缓冲区长度是安全的
	 */

	pjmedia_circ_buf_set_len(b->circ_buf, 
				 pjmedia_circ_buf_get_len(b->circ_buf) - 
				 erase_cnt);

	PJ_LOG(5,(b->obj_name,"%d samples reduced, buf_cnt=%d", 
	       erase_cnt, pjmedia_circ_buf_get_len(b->circ_buf)));
    }
}

/* 快升慢降 */
#define AGC_UP(cur, target) cur = (cur + target*3) >> 2
#define AGC_DOWN(cur, target) cur = (cur*3 + target) >> 2
#define AGC(cur, target) \
    if (cur < target) AGC_UP(cur, target); \
    else AGC_DOWN(cur, target)

static void update(pjmedia_delay_buf *b, enum OP op)
{
    /* 顺序操作 */
    if (op == b->last_op) {
	++b->level;
	return;
    } 

    /* 开关操作 */
    if (b->level > b->max_level)
	b->max_level = b->level;

    b->recalc_timer -= (b->level * b->ptime) >> 1;

    b->last_op = op;
    b->level = 1;

    /* 根据max_level重新计算有效计数 */
    if (b->recalc_timer <= 0) {
	unsigned new_eff_cnt = (b->max_level+SAFE_MARGIN)*b->samples_per_frame;

	/* 平滑有效计数转换 */
	AGC(b->eff_cnt, new_eff_cnt);
	
	/* 确保新的有效计数是通道计数的乘法，所以让我们将其取整
	 */
	if (b->eff_cnt % b->channel_count)
	    b->eff_cnt += b->channel_count - (b->eff_cnt % b->channel_count);

	TRACE__((b->obj_name,"Cur eff_cnt=%d", b->eff_cnt));
	
	b->max_level = 0;
	b->recalc_timer = RECALC_TIME;
    }

    /* 看看我们是否需要缩小缓冲区以减少延迟 */
    if (op == OP_PUT && pjmedia_circ_buf_get_len(b->circ_buf) > 
	b->samples_per_frame + b->eff_cnt)
    {
	unsigned erase_cnt = b->samples_per_frame >> 1;
	unsigned old_buf_cnt = pjmedia_circ_buf_get_len(b->circ_buf);

	shrink_buffer(b, erase_cnt);
	PJ_LOG(4,(b->obj_name,"Buffer size adjusted from %d to %d (eff_cnt=%d)",
	          old_buf_cnt,
		  pjmedia_circ_buf_get_len(b->circ_buf),
		  b->eff_cnt));
    }
}

PJ_DEF(pj_status_t) pjmedia_delay_buf_put(pjmedia_delay_buf *b,
					   pj_int16_t frame[])
{
    pj_status_t status;

    PJ_ASSERT_RETURN(b && frame, PJ_EINVAL);

    pj_lock_acquire(b->lock);

    if (b->wsola) {
        update(b, OP_PUT);
    
        status = pjmedia_wsola_save(b->wsola, frame, PJ_FALSE);
        if (status != PJ_SUCCESS) {
	    pj_lock_release(b->lock);
	    return status;
        }
    }

    /* 溢出检查 */
    if (pjmedia_circ_buf_get_len(b->circ_buf) + b->samples_per_frame > 
	b->max_cnt)
    {
	unsigned erase_cnt;

        if (b->wsola) {
	    /* 缩小一帧还是只差一帧？ */
	    //erase_cnt = b->samples_per_frame;
	    erase_cnt = pjmedia_circ_buf_get_len(b->circ_buf) + 
		        b->samples_per_frame - b->max_cnt;

	    shrink_buffer(b, erase_cnt);
        }

	/* 检查收缩失败或擦除计数是否小于请求的值，delaybuf需要删除最早的样本，这是不好的，因为语音样本得到粗糙的过渡，可能会产生滴答噪声
	 */
	if (pjmedia_circ_buf_get_len(b->circ_buf) + b->samples_per_frame > 
	    b->max_cnt) 
	{
	    erase_cnt = pjmedia_circ_buf_get_len(b->circ_buf) + 
			b->samples_per_frame - b->max_cnt;

	    pjmedia_circ_buf_adv_read_ptr(b->circ_buf, erase_cnt);

	    PJ_LOG(4,(b->obj_name,"%sDropping %d eldest samples, buf_cnt=%d",
                      (b->wsola? "Shrinking failed or insufficient. ": ""),
                      erase_cnt, pjmedia_circ_buf_get_len(b->circ_buf)));
	}
    }

    pjmedia_circ_buf_write(b->circ_buf, frame, b->samples_per_frame);

    pj_lock_release(b->lock);
    return PJ_SUCCESS;
}

PJ_DEF(pj_status_t) pjmedia_delay_buf_get( pjmedia_delay_buf *b,
					   pj_int16_t frame[])
{
    pj_status_t status = PJ_SUCCESS;

    PJ_ASSERT_RETURN(b && frame, PJ_EINVAL);

    pj_lock_acquire(b->lock);

    if (b->wsola)
        update(b, OP_GET);

    /* 饥饿检查 */
    if (pjmedia_circ_buf_get_len(b->circ_buf) < b->samples_per_frame) {

	PJ_LOG(4,(b->obj_name,"Underflow, buf_cnt=%d, will generate 1 frame",
		  pjmedia_circ_buf_get_len(b->circ_buf)));

        if (b->wsola) {
            status = pjmedia_wsola_generate(b->wsola, frame);

	    if (status == PJ_SUCCESS) {
	        TRACE__((b->obj_name,"Successfully generate 1 frame"));
	        if (pjmedia_circ_buf_get_len(b->circ_buf) == 0) {
		    pj_lock_release(b->lock);
		    return PJ_SUCCESS;
	        }

	        /* 将生成的帧放入缓冲区 */
	        pjmedia_circ_buf_write(b->circ_buf, frame,
                                       b->samples_per_frame);
            }
        }

	if (!b->wsola || status != PJ_SUCCESS) {
	    unsigned buf_len = pjmedia_circ_buf_get_len(b->circ_buf);
	    
	    /* 给出所有的延迟缓冲区，然后用零填充 */
            if (b->wsola)
	        PJ_LOG(4,(b->obj_name,"Error generating frame, status=%d", 
		          status));

	    pjmedia_circ_buf_read(b->circ_buf, frame, buf_len);
	    pjmedia_zero_samples(&frame[buf_len], 
				 b->samples_per_frame - buf_len);

	    /* 缓冲区现在是空的，请重置它 */
	    pjmedia_circ_buf_reset(b->circ_buf);

	    pj_lock_release(b->lock);

	    return PJ_SUCCESS;
	}
    }

    pjmedia_circ_buf_read(b->circ_buf, frame, b->samples_per_frame);

    pj_lock_release(b->lock);

    return PJ_SUCCESS;
}


PJ_DEF(pj_status_t) pjmedia_delay_buf_reset(pjmedia_delay_buf *b)
{
    PJ_ASSERT_RETURN(b, PJ_EINVAL);

    pj_lock_acquire(b->lock);

    b->recalc_timer = RECALC_TIME;

    /* 重置缓冲区 */
    pjmedia_circ_buf_reset(b->circ_buf);

    /* 重置 WSOLA */
    if (b->wsola)
        pjmedia_wsola_reset(b->wsola, 0);

    pj_lock_release(b->lock);

    PJ_LOG(5,(b->obj_name,"Delay buffer is reset"));

    return PJ_SUCCESS;
}

