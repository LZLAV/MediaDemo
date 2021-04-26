/**
 * 已完成：
 *
 */
#include <pjmedia/jbuf.h>
#include <pjmedia/errno.h>
#include <pj/pool.h>
#include <pj/assert.h>
#include <pj/log.h>
#include <pj/math.h>
#include <pj/string.h>


#define THIS_FILE   "jbuf.c"


/* 序列号无效，用作初始值 */
#define INVALID_OFFSET        -9999

/*
 * 最大突发长度，每当一个操作的突发长度大于此值时，JB将假定相反的操作处于空闲状态
 */
#define MAX_BURST_MSEC        1000

/* 在JB可以将其状态切换到JB_STATUS_PROCESSING之前，在JB_STATUS_INITIALIZING中要执行的OP切换数
 */
#define INIT_CYCLE        10


/*
 * JB帧列表中的最大帧索引（估计）。
 * 由于索引计算为 (RTP-timestamp/timestamp-span)，最大索引通常从 MAXUINT32/9000（90kHz时为10fps视频）到 MAXUINT32/80（8000Hz时为10ms音频），让我们取“最低”
 */
#define MAX_FRAME_INDEX        (0xFFFFFFFF/9000)


/*
 * 在静态丢弃算法中，JB size和2*burst-level之间的最小差异用于执行JB收缩
 */
#define STA_DISC_SAFE_SHRINKING_DIFF    1


/*
 * JB内部缓冲区的结构，用包含帧内容、帧类型、帧长度和帧位信息的循环缓冲区表示
 */
typedef struct jb_framelist_t {
    /* 设置 */
    unsigned frame_size;    /**< 帧的最大长度	    */
    unsigned max_count;        /**< 最大的帧数	    */

    /* 缓存 */
    char *content;        /**< 帧内容数组	    */
    int *frame_type;    /**< 帧类型数组		    */
    pj_size_t *content_len;    /**< 帧长度数组		    */
    pj_uint32_t *bit_info;        /**< 帧位信息数组	    */
    pj_uint32_t *ts;        /**< 时间戳数组		    */

    /* 状态 */
    unsigned head;        /**< 头的索引号，下一次GET 获取到的帧的指针 0 ~ (max_count-1)  */
    unsigned size;        /**< 帧列表的当前大小，包括丢弃的帧    */
    unsigned discarded_num;    /**< 当前丢弃的帧数		    */
    int origin;        /**< 在 flist_head中的原始索引：帧序列 */

} jb_framelist_t;


typedef void (*discard_algo)(pjmedia_jbuf *jb);

static void jbuf_discard_static(pjmedia_jbuf *jb);

static void jbuf_discard_progressive(pjmedia_jbuf *jb);


struct pjmedia_jbuf {
    /* 设置（常量） */
    pj_str_t jb_name;        /**< 抖动buf 名称	    */
    pj_size_t jb_frame_size;    /**< 帧大小			    */
    unsigned jb_frame_ptime;    /**< 帧长	    */
    pj_size_t jb_max_count;    /**< 抖动缓冲区容量，单位帧		    */
    int jb_init_prefetch;    /**< 初始预取值	    */
    int jb_min_prefetch;    /**< 最小允许预取值	    */
    int jb_max_prefetch;    /**< 最大允许预取	    */
    int jb_max_burst;    /**<    最大可能爆发，每当爆发超过这个值，它将不包括在水平计算 */
    int jb_min_shrink_gap;    /**< 收缩的间隔    */
    discard_algo jb_discard_algo;    /**< 丢弃算法  */

    /* 缓冲 */
    jb_framelist_t jb_framelist;    /**< 缓冲区  */

    /* 状态 */
    int jb_level;        /**< 源和目标之间的延迟（根据突发get/put操作数计算） */
    int jb_max_hist_level;  /**< 最近一次计算的最大级别 */
    int jb_stable_hist;    /**<    延迟低于预取延迟的次数 */
    int jb_last_op;        /**< 上一次执行的操作 */
    int jb_eff_level;    /**< 有效突发级别	    */
    int jb_prefetch;    /**<    在删除某些帧之前要插入的帧数（在 framelist->content 操作开始时），该值可能会根据当前帧突发级别不断更新 */
    pj_bool_t jb_prefetching;    /**< 标志jbuf是否为预取   */
    int jb_status;        /**< 状态为“init”，直到第一个“put”操作		    */
    int jb_init_cycle_cnt;    /**< 状态为“init”，直到第一个“put”操作		    */

    int jb_discard_ref;    /**< 删除或丢弃最近一帧的序列 */
    unsigned jb_discard_dist;    /**< 从 jb_discard_ref 到执行 discard 的距离（单位：frm） */

    /* 统计 */
    pj_math_stat jb_delay;        /**< 抖动缓冲器的延迟统计(in ms)			    */
    pj_math_stat jb_burst;        /**< 突发统计 (in frames)   */
    unsigned jb_lost;        /**< 丢失帧数	    */
    unsigned jb_discard;        /**< 丢弃的帧数    */
    unsigned jb_empty;        /**< GET返回的空/预取帧数 */
};


#define JB_STATUS_INITIALIZING    0
#define JB_STATUS_PROCESSING    1



/*
 * 提出了一种渐进丢弃算法，根据实际突发级别自适应地丢弃传入帧，以减少JB延迟
 */
#define PROGRESSIVE_DISCARD 1       //渐进式丢弃

/*
 * 内部JB frame标志，被丢弃的帧不会被JB返回给应用程序，它只是简单地被丢弃。
 */
#define PJMEDIA_JB_DISCARDED_FRAME 1024



/*
 * 启用此选项将每秒记录一次抖动缓冲区状态
 */
#if 0
#  define TRACE__(args)	    PJ_LOG(5,args)
#else
#  define TRACE__(args)
#endif

static pj_status_t jb_framelist_reset(jb_framelist_t *framelist);

static unsigned jb_framelist_remove_head(jb_framelist_t *framelist, unsigned count);

static pj_status_t jb_framelist_init(pj_pool_t *pool,
                                     jb_framelist_t *framelist,
                                     unsigned frame_size,
                                     unsigned max_count) {
    PJ_ASSERT_RETURN(pool && framelist, PJ_EINVAL);

    pj_bzero(framelist, sizeof(jb_framelist_t));

    framelist->frame_size = frame_size;
    framelist->max_count = max_count;
    framelist->content = (char *)
            pj_pool_alloc(pool,
                          framelist->frame_size *
                          framelist->max_count);
    framelist->frame_type = (int *)
            pj_pool_alloc(pool,
                          sizeof(framelist->frame_type[0]) *
                          framelist->max_count);
    framelist->content_len = (pj_size_t *)
            pj_pool_alloc(pool,
                          sizeof(framelist->content_len[0]) *
                          framelist->max_count);
    framelist->bit_info = (pj_uint32_t *)
            pj_pool_alloc(pool,
                          sizeof(framelist->bit_info[0]) *
                          framelist->max_count);
    framelist->ts = (pj_uint32_t *)
            pj_pool_alloc(pool,
                          sizeof(framelist->ts[0]) *
                          framelist->max_count);

    PJ_LOG(5, (THIS_FILE, "JB missing frame: jb_lost=%d", 2));
    return jb_framelist_reset(framelist);

}

static pj_status_t jb_framelist_destroy(jb_framelist_t *framelist) {
    PJ_UNUSED_ARG(framelist);
    return PJ_SUCCESS;
}

static pj_status_t jb_framelist_reset(jb_framelist_t *framelist) {
    framelist->head = 0;
    framelist->origin = INVALID_OFFSET;
    framelist->size = 0;
    framelist->discarded_num = 0;


    //pj_bzero(framelist->content,
    //	     framelist->frame_size *
    //	     framelist->max_count);

    pj_memset(framelist->frame_type,
              PJMEDIA_JB_MISSING_FRAME,
              sizeof(framelist->frame_type[0]) *
              framelist->max_count);

    pj_bzero(framelist->content_len,
             sizeof(framelist->content_len[0]) *
             framelist->max_count);

    //pj_bzero(framelist->bit_info,
    //	     sizeof(framelist->bit_info[0]) *
    //	     framelist->max_count);

    return PJ_SUCCESS;
}


static unsigned jb_framelist_size(const jb_framelist_t *framelist) {
    return framelist->size;
}


static unsigned jb_framelist_eff_size(const jb_framelist_t *framelist) {
    return (framelist->size - framelist->discarded_num);
}

static int jb_framelist_origin(const jb_framelist_t *framelist) {
    return framelist->origin;
}


static pj_bool_t jb_framelist_get(jb_framelist_t *framelist,
                                  void *frame, pj_size_t *size,
                                  pjmedia_jb_frame_type *p_type,
                                  pj_uint32_t *bit_info,
                                  pj_uint32_t *ts,
                                  int *seq) {
    if (framelist->size) {
        pj_bool_t prev_discarded = PJ_FALSE;

        /* 跳过丢弃的帧 */
        while (framelist->frame_type[framelist->head] ==
               PJMEDIA_JB_DISCARDED_FRAME) {
            jb_framelist_remove_head(framelist, 1);
            prev_discarded = PJ_TRUE;
        }

        /* 如有，返回头帧 */
        if (framelist->size) {
            if (prev_discarded) {
                /*
                 * #1188：当前一帧被丢弃时，返回 missing 帧以触发PLC以获得更平滑的信号。
                 */
                *p_type = PJMEDIA_JB_MISSING_FRAME;
                if (size)
                    *size = 0;
                if (bit_info)
                    *bit_info = 0;

                //PJ_LOG(5, (THIS_FILE, "JB missing frame: prev_discarded"));

            } else {
                pj_size_t frm_size = framelist->content_len[framelist->head];
                pj_size_t max_size = size ? *size : frm_size;
                pj_size_t copy_size = PJ_MIN(max_size, frm_size);

                /* 缓冲区大小不应小于帧大小 */
                if (max_size < frm_size) {
                    pj_assert(!"Buffer too small");
                    PJ_LOG(4, (THIS_FILE, "JB Warning: buffer too small for the retrieved frame!"));
                }

                pj_memcpy(frame,
                          framelist->content +
                          framelist->head * framelist->frame_size,
                          copy_size);
                *p_type = (pjmedia_jb_frame_type)
                        framelist->frame_type[framelist->head];//for now
                if (size)
                    *size = copy_size;
                if (bit_info)
                    *bit_info = framelist->bit_info[framelist->head];
            }
            if (ts)
                *ts = framelist->ts[framelist->head];
            if (seq)
                *seq = framelist->origin;

            //pj_bzero(framelist->content +
            //	 framelist->head * framelist->frame_size,
            //	 framelist->frame_size);
            framelist->frame_type[framelist->head] = PJMEDIA_JB_MISSING_FRAME;//for next,reset
            framelist->content_len[framelist->head] = 0;
            framelist->bit_info[framelist->head] = 0;
            framelist->ts[framelist->head] = 0;

            framelist->origin++;
            framelist->head = (framelist->head + 1) % framelist->max_count;
            framelist->size--;

            //PJ_LOG(5, (THIS_FILE, "JB missing frame: prev_discarded=%d(0.1), framelist:origin=%d, size=%d", prev_discarded, framelist->origin, framelist->size));//print too many

            return PJ_TRUE;
        }
    }

    /* 无帧可用 */
    pj_bzero(frame, framelist->frame_size);

    return PJ_FALSE;
}


static pj_bool_t jb_framelist_peek(jb_framelist_t *framelist,  //for  vid_stream
                                   unsigned offset,
                                   const void **frame,
                                   pj_size_t *size,
                                   pjmedia_jb_frame_type *type,
                                   pj_uint32_t *bit_info,
                                   pj_uint32_t *ts,
                                   int *seq) {
    unsigned pos, idx;

    if (offset >= jb_framelist_eff_size(framelist))
        return PJ_FALSE;

    pos = framelist->head;
    idx = offset;

    /* 找到实际的peek位置，注意可能有丢弃的帧 */
    while (1) {
        if (framelist->frame_type[pos] != PJMEDIA_JB_DISCARDED_FRAME) {
            if (idx == 0)
                break;
            else
                --idx;
        }
        pos = (pos + 1) % framelist->max_count;
    }

    /* 返回帧指针 */
    if (frame)
        *frame = framelist->content + pos * framelist->frame_size;
    if (type)
        *type = (pjmedia_jb_frame_type)
                framelist->frame_type[pos];
    if (size)
        *size = framelist->content_len[pos];
    if (bit_info)
        *bit_info = framelist->bit_info[pos];
    if (ts)
        *ts = framelist->ts[pos];
    if (seq)
        *seq = framelist->origin + offset;

    return PJ_TRUE;
}


/* 删除 count 个最旧的帧 */
static unsigned jb_framelist_remove_head(jb_framelist_t *framelist, unsigned count) {
    if (count > framelist->size)
        count = framelist->size;

    if (count) {
        /* 如果重叠，可分两步进行 */
        unsigned step1, step2;
        unsigned tmp = framelist->head + count;
        unsigned i;

        if (tmp > framelist->max_count) {
            step1 = framelist->max_count - framelist->head;
            step2 = count - step1;
        } else {
            step1 = count;
            step2 = 0;
        }

        for (i = framelist->head; i < (framelist->head + step1); ++i) {
            if (framelist->frame_type[i] == PJMEDIA_JB_DISCARDED_FRAME) {
                pj_assert(framelist->discarded_num > 0);
                framelist->discarded_num--;
            }
        }

        //pj_bzero(framelist->content +
        //	    framelist->head * framelist->frame_size,
        //          step1*framelist->frame_size);
        pj_memset(framelist->frame_type + framelist->head,
                  PJMEDIA_JB_MISSING_FRAME,
                  step1 * sizeof(framelist->frame_type[0]));
        pj_bzero(framelist->content_len + framelist->head,
                 step1 * sizeof(framelist->content_len[0]));

        if (step2) {
            for (i = 0; i < step2; ++i) {
                if (framelist->frame_type[i] == PJMEDIA_JB_DISCARDED_FRAME) {
                    pj_assert(framelist->discarded_num > 0);
                    framelist->discarded_num--;
                }
            }
            //pj_bzero( framelist->content,
            //	      step2*framelist->frame_size);
            pj_memset(framelist->frame_type,
                      PJMEDIA_JB_MISSING_FRAME,
                      step2 * sizeof(framelist->frame_type[0]));
            pj_bzero(framelist->content_len,
                     step2 * sizeof(framelist->content_len[0]));
        }

        /* 更新状态 */
        framelist->origin += count;
        framelist->head = (framelist->head + count) % framelist->max_count;
        framelist->size -= count;
    }

    return count;
}


static pj_status_t jb_framelist_put_at(jb_framelist_t *framelist,
                                       int index,             //frame seq //maojh
                                       const void *frame,
                                       unsigned frame_size,
                                       pj_uint32_t bit_info,
                                       pj_uint32_t ts,
                                       unsigned frame_type) {
    int distance;
    unsigned pos;
    enum {
        MAX_MISORDER = 100
    };
    enum {
        MAX_DROPOUT = 3000
    };

    PJ_ASSERT_RETURN(frame_size <= framelist->frame_size, PJ_EINVAL);

    /* 获取此帧到缓冲区中第一帧的距离 */
    distance = index - framelist->origin;

    /* 太晚或序列重新启动或跳远 */
    if (distance < 0) {
        if (framelist->origin - index < MAX_MISORDER) {
            /* 太晚 */
            /*TRACE__(*/PJ_LOG(4, (THIS_FILE, "JB Put frame #%d: too late (distance=%d)",
                    index, distance));
            return PJ_ETOOSMALL;
        } else if (framelist->origin + framelist->size >= MAX_FRAME_INDEX) {
            /* 序列重新开始 */
            /*TRACE__(*/PJ_LOG(4, (THIS_FILE, "JB Put frame #%d: sequence restart (distance=%d, "
                                              "orig=%d, size=%d)",
                    index, distance, framelist->origin,
                    framelist->size));
            framelist->origin = index - framelist->size;
            distance = framelist->size;
        } else {
            /* 跳得太远，重置缓冲区 */
            /*TRACE__(*/PJ_LOG(4, (THIS_FILE, "JB Put frame #%d: far jump (distance=%d)",
                    index, distance));
            jb_framelist_reset(framelist);
            framelist->origin = index;
            distance = 0;
        }
    }

    /* 如果jbuf为空，只需重置 origin */
    if (framelist->size == 0) {
        pj_assert(framelist->discarded_num == 0);
        TRACE__((THIS_FILE, "JB Put frame #%d: origin reset (from %d) as JB empty",
                index, framelist->origin));//print too many at the begining.//maojh
        framelist->origin = index;
        distance = 0;
    }

    /* jump太远，距离大于缓冲容量 */
    if (distance >= (int) framelist->max_count) {
        if (distance > MAX_DROPOUT) {
            /* jump太远，重置 buffer */
            /*TRACE__(*/PJ_LOG(4, (THIS_FILE, "JB Put frame #%d: far jump (distance=%d)",
                    index, distance));
            jb_framelist_reset(framelist);
            framelist->origin = index;
            distance = 0;
        } else {
            /* 否则，拒绝帧*/
            TRACE__((THIS_FILE, "JB Put frame #%d: rejected due to JB full",
                    index));//if not listening, print too many. //maojh
            return PJ_ETOOMANY;
        }
    }

    /* 获取slot 位置 */
    pos = (framelist->head + distance) % framelist->max_count;

    /* 如果slot被占用，它必须是复制帧，忽略它 */
    if (framelist->frame_type[pos] != PJMEDIA_JB_MISSING_FRAME) {
        /*TRACE__(*/PJ_LOG(4, (THIS_FILE, "JB Put frame #%d maybe a duplicate, ignored", index));
        return PJ_EEXISTS;
    }

    /* 放一帧到 slot */
    framelist->frame_type[pos] = frame_type;
    framelist->content_len[pos] = frame_size;
    framelist->bit_info[pos] = bit_info;
    framelist->ts[pos] = ts;

    /* 更新 framelist size */
    if (framelist->origin + (int) framelist->size <= index)
        framelist->size = distance + 1;

    if (PJMEDIA_JB_NORMAL_FRAME == frame_type) {
        /* 拷贝帧内容 */
        pj_memcpy(framelist->content + pos * framelist->frame_size,
                  frame, frame_size);
    }

    return PJ_SUCCESS;
}


static pj_status_t jb_framelist_discard(jb_framelist_t *framelist,
                                        int index) {
    unsigned pos;

    PJ_ASSERT_RETURN(index >= framelist->origin &&
                     index < framelist->origin + (int) framelist->size,
                     PJ_EINVAL);

    /* 获得 slot 位置 */
    pos = (framelist->head + (index - framelist->origin)) %
          framelist->max_count;

    /* 丢弃帧 */
    framelist->frame_type[pos] = PJMEDIA_JB_DISCARDED_FRAME;
    framelist->discarded_num++;

    if ((pos % 5 == 0) || (framelist->discarded_num > 2)) {
        PJ_LOG(4,
               (THIS_FILE, "JB discard:  pos=%d(mod5), or, discarded_num=%d(>2)", pos, framelist->discarded_num));
    }

    return PJ_SUCCESS;
}


enum pjmedia_jb_op {
    JB_OP_INIT = -1,
    JB_OP_PUT = 1,
    JB_OP_GET = 2
};


PJ_DEF(pj_status_t) pjmedia_jbuf_create(pj_pool_t *pool,
                                        const pj_str_t *name,
                                        unsigned frame_size,
                                        unsigned ptime,
                                        unsigned max_count,
                                        pjmedia_jbuf **p_jb) {
    pjmedia_jbuf *jb;
    pj_status_t status;

    jb = PJ_POOL_ZALLOC_T(pool, pjmedia_jbuf);

    status = jb_framelist_init(pool, &jb->jb_framelist, frame_size, max_count);
    if (status != PJ_SUCCESS)
        return status;

    pj_strdup_with_null(pool, &jb->jb_name, name);
    jb->jb_frame_size = frame_size;
    jb->jb_frame_ptime = ptime;
    jb->jb_prefetch = PJ_MIN(PJMEDIA_JB_DEFAULT_INIT_DELAY, max_count * 4 / 5);
    jb->jb_min_prefetch = 0;
    jb->jb_max_prefetch = max_count * 4 / 5;
    jb->jb_max_count = max_count;
    jb->jb_min_shrink_gap = PJMEDIA_JBUF_DISC_MIN_GAP / ptime;
    jb->jb_max_burst = PJ_MAX(MAX_BURST_MSEC / ptime, max_count * 3 / 4);

    pj_math_stat_init(&jb->jb_delay);
    pj_math_stat_init(&jb->jb_burst);

    pjmedia_jbuf_set_discard(jb, PJMEDIA_JB_DISCARD_PROGRESSIVE);
    pjmedia_jbuf_reset(jb);

    *p_jb = jb;

    PJ_LOG(4, (THIS_FILE, "JB init:\n"
                          "  max_count=%d(50f), frame_size=%d(80B), ptime=%d(10ms), (pre/min/max)prefetch=%d(15f)/%d(0f)/%d(40f)",
            jb->jb_max_count, jb->jb_frame_size, jb->jb_frame_ptime, jb->jb_prefetch, jb->jb_min_prefetch, jb->jb_max_prefetch));
    return PJ_SUCCESS;
}


PJ_DEF(pj_status_t) pjmedia_jbuf_set_ptime(pjmedia_jbuf *jb,
                                           unsigned ptime) {
    PJ_ASSERT_RETURN(jb, PJ_EINVAL);

    jb->jb_frame_ptime = ptime;
    jb->jb_min_shrink_gap = PJMEDIA_JBUF_DISC_MIN_GAP / ptime;      //最小收缩间隔 200/ptime
    jb->jb_max_burst = PJ_MAX(MAX_BURST_MSEC / ptime,
                              jb->jb_max_count * 3 / 4);        //最大的 burst max(1000/ptime,max_count*3/4)

    return PJ_SUCCESS;
}


/*
 * 将抖动缓冲设置为固定延迟模式。默认行为是根据实际的数据包延迟调整延迟
 */
PJ_DEF(pj_status_t) pjmedia_jbuf_set_fixed(pjmedia_jbuf *jb,
                                           unsigned prefetch) {
    PJ_ASSERT_RETURN(jb, PJ_EINVAL);
    PJ_ASSERT_RETURN(prefetch <= jb->jb_max_count, PJ_EINVAL);

    jb->jb_min_prefetch = jb->jb_max_prefetch =
    jb->jb_prefetch = jb->jb_init_prefetch = prefetch;

    pjmedia_jbuf_set_discard(jb, PJMEDIA_JB_DISCARD_NONE);
    return PJ_SUCCESS;
}


/*
 * 将抖动缓冲区设置为自适应模式
 */
PJ_DEF(pj_status_t) pjmedia_jbuf_set_adaptive(pjmedia_jbuf *jb,
                                              unsigned prefetch,
                                              unsigned min_prefetch,
                                              unsigned max_prefetch) {
    PJ_ASSERT_RETURN(jb, PJ_EINVAL);
    PJ_ASSERT_RETURN(min_prefetch <= max_prefetch &&
                     prefetch <= max_prefetch &&
                     max_prefetch <= jb->jb_max_count,
                     PJ_EINVAL);

    jb->jb_prefetch = jb->jb_init_prefetch = prefetch;
    jb->jb_min_prefetch = min_prefetch;
    jb->jb_max_prefetch = max_prefetch;

    PJ_LOG(4, (THIS_FILE, "JB init:\n"
                          "  adaptive (init/min/max)prefetch=%d/%d/%d frames", prefetch, min_prefetch, max_prefetch));
    return PJ_SUCCESS;
}

/**
 * 设置丢弃算法
 * @param jb
 * @param algo
 * @return
 */
PJ_DEF(pj_status_t) pjmedia_jbuf_set_discard(pjmedia_jbuf *jb,
                                             pjmedia_jb_discard_algo algo) {
    PJ_ASSERT_RETURN(jb, PJ_EINVAL);
    PJ_ASSERT_RETURN(algo >= PJMEDIA_JB_DISCARD_NONE &&
                     algo <= PJMEDIA_JB_DISCARD_PROGRESSIVE,
                     PJ_EINVAL);

    switch (algo) {
        case PJMEDIA_JB_DISCARD_PROGRESSIVE:
            jb->jb_discard_algo = &jbuf_discard_progressive;
            break;
        case PJMEDIA_JB_DISCARD_STATIC:
            jb->jb_discard_algo = &jbuf_discard_static;
            break;
        default:
            jb->jb_discard_algo = NULL;
            break;
    }

    return PJ_SUCCESS;
}


PJ_DEF(pj_status_t) pjmedia_jbuf_reset(pjmedia_jbuf *jb) {
    jb->jb_level = 0;
    jb->jb_last_op = JB_OP_INIT;
    jb->jb_stable_hist = 0;
    jb->jb_status = JB_STATUS_INITIALIZING;
    jb->jb_init_cycle_cnt = 0;
    jb->jb_max_hist_level = 0;
    jb->jb_prefetching = (jb->jb_prefetch != 0);
    jb->jb_discard_dist = 0;

    jb_framelist_reset(&jb->jb_framelist);

    return PJ_SUCCESS;
}


PJ_DEF(pj_status_t) pjmedia_jbuf_destroy(pjmedia_jbuf *jb) {
    PJ_LOG(5, (jb->jb_name.ptr, ""
                                "JB summary:\n"
                                "  size=%d/eff=%d prefetch=%d level=%d\n"
                                "  delay (min/max/avg/dev)=%d/%d/%d/%d ms\n"
                                "  burst (min/max/avg/dev)=%d/%d/%d/%d frames\n"
                                "  lost=%d discard=%d empty=%d",
            jb_framelist_size(&jb->jb_framelist),
            jb_framelist_eff_size(&jb->jb_framelist),
            jb->jb_prefetch, jb->jb_eff_level,
            jb->jb_delay.min, jb->jb_delay.max, jb->jb_delay.mean,
            pj_math_stat_get_stddev(&jb->jb_delay),
            jb->jb_burst.min, jb->jb_burst.max, jb->jb_burst.mean,
            pj_math_stat_get_stddev(&jb->jb_burst),
            jb->jb_lost, jb->jb_discard, jb->jb_empty));

    return jb_framelist_destroy(&jb->jb_framelist);
}

PJ_DEF(pj_bool_t) pjmedia_jbuf_is_full(const pjmedia_jbuf *jb) {
    return jb->jb_framelist.size == jb->jb_framelist.max_count;
}

/**
 * 计算抖动
 * @param jb
 */
static void jbuf_calculate_jitter(pjmedia_jbuf *jb) {
    int diff, cur_size;

    cur_size = jb_framelist_eff_size(&jb->jb_framelist);
    pj_math_stat_update(&jb->jb_burst, jb->jb_level);
    jb->jb_max_hist_level = PJ_MAX(jb->jb_max_hist_level, jb->jb_level);

    /* 突发级别正在降低 */
    if (jb->jb_level < jb->jb_eff_level) {

        enum {
            STABLE_HISTORY_LIMIT = 20
        };

        jb->jb_stable_hist++;

        /* 仅在达到“稳定”条件（而不仅仅是短时脉冲）时才更新有效电平（和预取）
         */
        if (jb->jb_stable_hist > STABLE_HISTORY_LIMIT) {

            diff = (jb->jb_eff_level - jb->jb_max_hist_level) / 3;

            if (diff < 1)
                diff = 1;

            /* 更新有效突发水平 */
            jb->jb_eff_level -= diff;

            /* 基于级别更新预取 */
            if (jb->jb_init_prefetch) {
                jb->jb_prefetch = jb->jb_eff_level;
                if (jb->jb_prefetch < jb->jb_min_prefetch)
                    jb->jb_prefetch = jb->jb_min_prefetch;
                if (jb->jb_prefetch > jb->jb_max_prefetch)
                    jb->jb_prefetch = jb->jb_max_prefetch;
            }

            /* 重置历史数据 */
            jb->jb_max_hist_level = 0;
            jb->jb_stable_hist = 0;

            TRACE__((jb->jb_name.ptr, "JB updated(1), level=%d pre=%d, size=%d",
                    jb->jb_eff_level, jb->jb_prefetch, cur_size));
            PJ_UNUSED_ARG(cur_size); /* Warning about unused var */
        }
    }

        /* 突发级别正在增加 */
    else if (jb->jb_level > jb->jb_eff_level) {

        /* 瞬时将有效突发电平设置为最近的最大电平 */
        jb->jb_eff_level = PJ_MIN(jb->jb_max_hist_level,
                                  (int) (jb->jb_max_count * 4 / 5));

        /* 基于级别更新预取 */
        if (jb->jb_init_prefetch) {
            jb->jb_prefetch = jb->jb_eff_level;
            if (jb->jb_prefetch > jb->jb_max_prefetch)
                jb->jb_prefetch = jb->jb_max_prefetch;
            if (jb->jb_prefetch < jb->jb_min_prefetch)
                jb->jb_prefetch = jb->jb_min_prefetch;
        }

        jb->jb_stable_hist = 0;
        /* 不要重置 max_hist_level */
        //jb->jb_max_hist_level = 0;

        TRACE__((jb->jb_name.ptr, "JB updated(2), level=%d pre=%d, size=%d",
                jb->jb_eff_level, jb->jb_prefetch, cur_size));
    }

        /* 级别不变 */
    else {
        jb->jb_stable_hist = 0;
    }
}


static void jbuf_discard_static(pjmedia_jbuf *jb) {
    /*
     * 这些代码用于缩短抖动缓冲区中的延迟。
     * 只有在有漂移的可能时才需要收缩。漂移检测是通过检测抖动缓冲区的大小来实现的，如果抖动缓冲区的大小是当前突发电平的两倍，则会出现漂移
     * 此外，通常漂移电平很低，因此JB不需要剧烈收缩，它将使每个 PJMEDIA_JBUF_DISC_MIN_GAP ms最多收缩一帧。理论上，
     * JB可以处理漂移电平为=FRAME_PTIME/PJMEDIA_JBUF_DISC_MIN_GAP * 100%
     *
     * 每当出现漂移时，在 PUT>GET 处，这种方法将保持延迟（JB大小）为突发级别的两倍
     * 由于漂移而导致的收缩将由渐进丢弃隐式完成，所以只要在渐进丢弃处于活动状态时禁用它即可
     */
    int diff, burst_level;

    burst_level = PJ_MAX(jb->jb_eff_level, jb->jb_level);
    diff = jb_framelist_eff_size(&jb->jb_framelist) - burst_level * 2;

    if (diff >= STA_DISC_SAFE_SHRINKING_DIFF) {
        int seq_origin;

        /* 检查并调整jb_discard_ref，以防出现 seq 重启
         */
        seq_origin = jb_framelist_origin(&jb->jb_framelist);
        if (seq_origin < jb->jb_discard_ref)
            jb->jb_discard_ref = seq_origin;

        if (seq_origin - jb->jb_discard_ref >= jb->jb_min_shrink_gap) {
            /* 缓慢收缩，每周期一帧 */
            diff = 1;

            /* 丢帧 */
            diff = jb_framelist_remove_head(&jb->jb_framelist, diff);
            jb->jb_discard_ref = jb_framelist_origin(&jb->jb_framelist);
            jb->jb_discard += diff;

            TRACE__((jb->jb_name.ptr,
                    "JB shrinking %d frame(s), cur size=%d", diff,
                    jb_framelist_eff_size(&jb->jb_framelist)));
        }
    }
}


static void jbuf_discard_progressive(pjmedia_jbuf *jb) {
    unsigned cur_size, burst_level, overflow, T, discard_dist;
    int last_seq;

    /*PUT操作已经完成 */
    if (jb->jb_last_op != JB_OP_PUT)
        return;

    /* 检查延迟是否长于突发 */
    cur_size = jb_framelist_eff_size(&jb->jb_framelist);
    burst_level = PJ_MAX(jb->jb_eff_level, jb->jb_level);
    if (cur_size <= burst_level) {
        /* 重置丢弃计划 */
        jb->jb_discard_dist = 0;
        return;
    }

    /* 估计调整延迟所需的丢弃持续时间 */
    if (burst_level <= PJMEDIA_JBUF_PRO_DISC_MIN_BURST)
        T = PJMEDIA_JBUF_PRO_DISC_T1;
    else if (burst_level >= PJMEDIA_JBUF_PRO_DISC_MAX_BURST)
        T = PJMEDIA_JBUF_PRO_DISC_T2;
    else
        T = PJMEDIA_JBUF_PRO_DISC_T1 +
            (PJMEDIA_JBUF_PRO_DISC_T2 - PJMEDIA_JBUF_PRO_DISC_T1) *
            (burst_level - PJMEDIA_JBUF_PRO_DISC_MIN_BURST) /
            (PJMEDIA_JBUF_PRO_DISC_MAX_BURST - PJMEDIA_JBUF_PRO_DISC_MIN_BURST);

    /* 计算当前丢弃距离 */
    overflow = cur_size - burst_level;
    discard_dist = T / overflow / jb->jb_frame_ptime;

    /* 获取JB中的最后一个序列号 */
    last_seq = jb_framelist_origin(&jb->jb_framelist) +
               jb_framelist_size(&jb->jb_framelist) - 1;

    /* 如果没有则设置新的丢弃计划，否则，更新现有的放弃计划（可以延迟或加速）
     */
    if (jb->jb_discard_dist == 0) {
        /* 设置新的放弃计划 */
        jb->jb_discard_ref = last_seq;
    } else if (last_seq < jb->jb_discard_ref) {
        /* 序列重启，更新丢弃引用 */
        jb->jb_discard_ref = last_seq;
    }
    jb->jb_discard_dist = PJ_MAX(jb->jb_min_shrink_gap, (int) discard_dist);

    /* 检查是否需要丢弃 */
    if (last_seq >= (jb->jb_discard_ref + (int) jb->jb_discard_dist)) {
        int discard_seq;

        discard_seq = jb->jb_discard_ref + jb->jb_discard_dist;
        if (discard_seq < jb_framelist_origin(&jb->jb_framelist))
            discard_seq = jb_framelist_origin(&jb->jb_framelist);

        jb_framelist_discard(&jb->jb_framelist, discard_seq);

        TRACE__((jb->jb_name.ptr,
                "JB Discard #%d: ref=#%d dist=%d orig=%d size=%d/%d "
                "burst=%d/%d",
                discard_seq,
                jb->jb_discard_ref,
                jb->jb_discard_dist,
                jb_framelist_origin(&jb->jb_framelist),
                cur_size,
                jb_framelist_size(&jb->jb_framelist),
                jb->jb_eff_level,
                burst_level));

        /* 更新丢弃引用 */
        jb->jb_discard_ref = discard_seq;
    }
}


PJ_INLINE(void) jbuf_update(pjmedia_jbuf *jb, int oper) {
    if (jb->jb_last_op != oper) {
        jb->jb_last_op = oper;

        if (jb->jb_status == JB_STATUS_INITIALIZING) {
            /*
             * 切换状态'initializing' -> 'processing' 在一些 OP 切换周期之后，并且当前 OP 是GET（突发电平基于 PUT突发计算），
             * 因此保证在状态切换之后立即执行突发计算
             */
            if (++jb->jb_init_cycle_cnt >= INIT_CYCLE && oper == JB_OP_GET) {
                jb->jb_status = JB_STATUS_PROCESSING;
                /* 为确保在此之后立即进行突发计算，如果突发级别超过最大突发级别，请调整突发级别
                 */
                jb->jb_level = PJ_MIN(jb->jb_level, jb->jb_max_burst);
            } else {
                jb->jb_level = 0;
                return;
            }
        }

        /*
         * 仅基于 PUT 突发级别执行抖动计算，因为 GET 突发级别可能不准确，例如：当VAD处于活动状态时
         * 请注意，当burst-level 太大时，即：超过jb_max_bursts，GET op可能处于空闲状态，在这种情况下，我们最好跳过抖动计算
         */
        if (oper == JB_OP_GET && jb->jb_level <= jb->jb_max_burst)
            jbuf_calculate_jitter(jb);

        jb->jb_level = 0;
    }

    /* 调用丢弃算法 */
    if (jb->jb_status == JB_STATUS_PROCESSING && jb->jb_discard_algo) {
        (*jb->jb_discard_algo)(jb);
    }
}

PJ_DEF(void) pjmedia_jbuf_put_frame(pjmedia_jbuf *jb,
                                    const void *frame,
                                    pj_size_t frame_size,
                                    int frame_seq) {
    pjmedia_jbuf_put_frame3(jb, frame, frame_size, 0, frame_seq, 0, NULL);
}

PJ_DEF(void) pjmedia_jbuf_put_frame2(pjmedia_jbuf *jb,
                                     const void *frame,
                                     pj_size_t frame_size,
                                     pj_uint32_t bit_info,
                                     int frame_seq,
                                     pj_bool_t *discarded) {
    pjmedia_jbuf_put_frame3(jb, frame, frame_size, bit_info, frame_seq, 0,
                            discarded);
}

PJ_DEF(void) pjmedia_jbuf_put_frame3(pjmedia_jbuf *jb,
                                     const void *frame,
                                     pj_size_t frame_size,
                                     pj_uint32_t bit_info,
                                     int frame_seq,  //frame seq
                                     pj_uint32_t ts,
                                     pj_bool_t *discarded) {
    pj_size_t min_frame_size;
    int new_size, cur_size;
    pj_status_t status;

    cur_size = jb_framelist_eff_size(&jb->jb_framelist);

    /* 检查帧的大小是否大于JB 帧 size  */
    if (frame_size > jb->jb_frame_size) {
        PJ_LOG(4,
               (THIS_FILE, "JB Warning: frame too large for jitter buffer, (( frame_size=%d > jb_frame_size=%d )), it will be truncated!", frame_size, jb->jb_frame_size));
    }

    /* 尝试存储帧 */
    min_frame_size = PJ_MIN(frame_size, jb->jb_frame_size);
    status = jb_framelist_put_at(&jb->jb_framelist, frame_seq, frame,
                                 (unsigned) min_frame_size, bit_info, ts,
                                 PJMEDIA_JB_NORMAL_FRAME);

    /* 抖动缓冲区已满，请删除一些旧帧 */
    while (status == PJ_ETOOMANY) {
        int distance;
        unsigned removed;

        /*
         * 移除尽可能少，仅仅是为了放入该帧
         * 请注意，seq-jump、out-of-order 和 seq restart等情况应该先由 jb_framelist_put_at() 处理/规范化。因此我们对这里的'distance'值很有信心
         */
        distance = (frame_seq - jb_framelist_origin(&jb->jb_framelist)) -
                   (int) jb->jb_max_count + 1;
        pj_assert(distance > 0);

        removed = jb_framelist_remove_head(&jb->jb_framelist, distance);
        status = jb_framelist_put_at(&jb->jb_framelist, frame_seq, frame,
                                     (unsigned) min_frame_size, bit_info, ts,
                                     PJMEDIA_JB_NORMAL_FRAME);

        jb->jb_discard += removed;
    }

    /* 在PUT 之后获取一个新的 JB size */
    new_size = jb_framelist_eff_size(&jb->jb_framelist);

    /* 如果丢弃此帧，则返回标志 */
    if (discarded)
        *discarded = (status != PJ_SUCCESS);

    if (status == PJ_SUCCESS) {
        if (jb->jb_prefetching) {
            TRACE__((jb->jb_name.ptr, "JB PUT prefetch_cnt=%d/%d",
                    new_size, jb->jb_prefetch));
            if (new_size >= jb->jb_prefetch)
                jb->jb_prefetching = PJ_FALSE;
        }
        jb->jb_level += (new_size > cur_size ? new_size - cur_size : 1);
        jbuf_update(jb, JB_OP_PUT);
    } else
        jb->jb_discard++;
}

/*
 * 从抖动缓冲区获取帧
 */
PJ_DEF(void) pjmedia_jbuf_get_frame(pjmedia_jbuf *jb,
                                    void *frame,
                                    char *p_frame_type) {
    pjmedia_jbuf_get_frame3(jb, frame, NULL, p_frame_type, NULL,
                            NULL, NULL);
}

/*
 * 从抖动缓冲区获取帧
 */
PJ_DEF(void) pjmedia_jbuf_get_frame2(pjmedia_jbuf *jb,
                                     void *frame,
                                     pj_size_t *size,
                                     char *p_frame_type,
                                     pj_uint32_t *bit_info) {
    pjmedia_jbuf_get_frame3(jb, frame, size, p_frame_type, bit_info,
                            NULL, NULL);
}

/*
 * 从抖动缓冲区获取帧
 */
PJ_DEF(void) pjmedia_jbuf_get_frame3(pjmedia_jbuf *jb,
                                     void *frame,
                                     pj_size_t *size,
                                     char *p_frame_type,
                                     pj_uint32_t *bit_info,
                                     pj_uint32_t *ts,
                                     int *seq) {
    if (jb->jb_prefetching) {

        /*
         * 无法返回帧，因为抖动缓冲区正在填充最小预取
         */

        //pj_bzero(frame, jb->jb_frame_size);
        *p_frame_type = PJMEDIA_JB_ZERO_PREFETCH_FRAME;
        if (size)
            *size = 0;

        TRACE__((jb->jb_name.ptr, "JB GET prefetch_cnt=%d/%d",
                jb_framelist_eff_size(&jb->jb_framelist), jb->jb_prefetch));

        jb->jb_empty++;

    } else {

        pjmedia_jb_frame_type ftype = PJMEDIA_JB_NORMAL_FRAME;
        pj_bool_t res;

        /* 尝试从 framelist 中检索帧 */
        res = jb_framelist_get(&jb->jb_framelist, frame, size, &ftype,
                               bit_info, ts, seq);
        if (res) {
            /* 我们已成功从帧列表中检索到帧，但该帧可能是空白帧
             */
            if (ftype == PJMEDIA_JB_NORMAL_FRAME) {
                *p_frame_type = PJMEDIA_JB_NORMAL_FRAME;
            } else {
                *p_frame_type = PJMEDIA_JB_MISSING_FRAME;
                jb->jb_lost++;

                if (jb->jb_lost % 50 == 0) {
                    PJ_LOG(5, (THIS_FILE, "JB missing frame: jb_lost=%d(mod50)", jb->jb_lost));
                }
            }

            /* 首次获取时存储延迟历史记录 */
            if (jb->jb_last_op == JB_OP_PUT) {
                unsigned cur_size;

                /* 我们刚检索到一帧，所以添加一个到cur_size */
                cur_size = jb_framelist_eff_size(&jb->jb_framelist) + 1;
                pj_math_stat_update(&jb->jb_delay,
                                    cur_size * jb->jb_frame_ptime);
            }
        } else {
            /* 抖动缓冲区为空 */
            if (jb->jb_prefetch)
                jb->jb_prefetching = PJ_TRUE;

            //pj_bzero(frame, jb->jb_frame_size);
            *p_frame_type = PJMEDIA_JB_ZERO_EMPTY_FRAME;
            if (size)
                *size = 0;

            jb->jb_empty++;
        }//else
    }//else

    jb->jb_level++;
    jbuf_update(jb, JB_OP_GET);
}

/*
 * 获取抖动缓冲区状态
 */
PJ_DEF(pj_status_t) pjmedia_jbuf_get_state(const pjmedia_jbuf *jb,
                                           pjmedia_jb_state *state) {
    PJ_ASSERT_RETURN(jb && state, PJ_EINVAL);

    state->frame_size = (unsigned) jb->jb_frame_size;
    state->min_prefetch = jb->jb_min_prefetch;
    state->max_prefetch = jb->jb_max_prefetch;

    state->burst = jb->jb_eff_level;
    state->prefetch = jb->jb_prefetch;
    state->size = jb_framelist_eff_size(&jb->jb_framelist);

    state->avg_delay = jb->jb_delay.mean;
    state->min_delay = jb->jb_delay.min;
    state->max_delay = jb->jb_delay.max;
    state->dev_delay = pj_math_stat_get_stddev(&jb->jb_delay);

    state->avg_burst = jb->jb_burst.mean;
    state->empty = jb->jb_empty;
    state->discard = jb->jb_discard;
    state->lost = jb->jb_lost;

    return PJ_SUCCESS;
}


PJ_DEF(void) pjmedia_jbuf_peek_frame(pjmedia_jbuf *jb,  //for  vid_stream
                                     unsigned offset,
                                     const void **frame,
                                     pj_size_t *size,
                                     char *p_frm_type,
                                     pj_uint32_t *bit_info,
                                     pj_uint32_t *ts,
                                     int *seq) {
    pjmedia_jb_frame_type ftype;
    pj_bool_t res;

    res = jb_framelist_peek(&jb->jb_framelist, offset, frame, size, &ftype,
                            bit_info, ts, seq);
    if (!res)
        *p_frm_type = PJMEDIA_JB_ZERO_EMPTY_FRAME;
    else if (ftype == PJMEDIA_JB_NORMAL_FRAME)
        *p_frm_type = PJMEDIA_JB_NORMAL_FRAME;
    else {
        *p_frm_type = PJMEDIA_JB_MISSING_FRAME;

        PJ_LOG(5,
               (THIS_FILE, "JB missing frame: pjmedia_jbuf_peek_frame, framelist:origin=%d, size=%d", jb->jb_framelist.origin, jb->jb_framelist.size));
    }
}


PJ_DEF(unsigned) pjmedia_jbuf_remove_frame(pjmedia_jbuf *jb,  //for  vid_stream
                                           unsigned frame_cnt) {
    unsigned count, last_discard_num;

    last_discard_num = jb->jb_framelist.discarded_num;
    count = jb_framelist_remove_head(&jb->jb_framelist, frame_cnt);

    /* 当包含丢弃的帧时，再删除一些 */
    while (jb->jb_framelist.discarded_num < last_discard_num) {
        /* 计算下一步要删除的帧数 */
        frame_cnt = last_discard_num - jb->jb_framelist.discarded_num;

        /* 刚刚被移除的正常不丢弃帧数 */
        count -= frame_cnt;

        /* 移除更多帧 */
        last_discard_num = jb->jb_framelist.discarded_num;
        count += jb_framelist_remove_head(&jb->jb_framelist, frame_cnt);
    }

    return count;
}
