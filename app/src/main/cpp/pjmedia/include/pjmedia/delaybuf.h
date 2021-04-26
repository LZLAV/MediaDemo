/**
 * 已完成
 * 		延迟缓冲区：
 * 			基本API:
 * 				create()
 * 				put()
 * 				get()
 * 				reset()			//重置 delaybuf，但学习状态保留
 * 				destroy()
 * 			学习过程：
 *
 *
 */

#ifndef __PJMEDIA_DELAYBUF_H__
#define __PJMEDIA_DELAYBUF_H__


/**
 * @file delaybuf.h
 * @brief 延迟buf
 */

#include <pjmedia/types.h>

/**
 * @defgroup PJMED_DELAYBUF 自适应延迟缓冲器
 * @ingroup PJMEDIA_FRAME_OP
 * @brief 高质量时间-刻度的自适应延迟缓冲器
 * 修改
 * @{
 *
 * 本节描述PJMEDIA对延迟缓冲区的实现。
 * 延迟缓冲器的工作原理与固定抖动缓冲器非常相似，即它将帧检索延迟一段时间，以便调用者从缓冲器中获得连续帧。这在 put()和get()操作不是均匀交错的情况下非常有用，
 * 例如调用方执行put()操作突发，然后执行get()操作突发。使用此延迟缓冲区时，缓冲区会将突发帧放入缓冲区，以便get()操作总是从缓冲区中获取帧（假设get()和put()的数目匹配）。
 *
 * 缓冲区是自适应的，也就是说，它在运行时不断学习应用于音频流的最佳延迟。一旦学习到最佳延迟，延迟缓冲器将对音频流应用该延迟，当缓冲器中的实际音频样本过低或过高时，根据需要扩展或收缩音频样本。
 * 它通过使用 PJMED_WSOLA 在不失真音频质量的情况下实现这一点。
 *
 * 延迟缓冲区用于PJMED_SND_PORT、PJMEDIA_SPLITCOMB 和PJMEDIA_CONF
 */

PJ_BEGIN_DECL

/** 延迟缓冲区的不透明声明 */
typedef struct pjmedia_delay_buf pjmedia_delay_buf;

/**
 * 延迟缓冲区选项
 */
typedef enum pjmedia_delay_buf_flag
{
    /**
     * 延迟缓冲区采用简单的FIFO机制，即无 WSOLA来扩展和缩小音频样本
     */
    PJMEDIA_DELAY_BUF_SIMPLE_FIFO = 1

} pjmedia_delay_buf_flag;

/**
 * 创建延迟缓冲区。一旦创建了延迟缓冲区，它将进入学习状态，除非指定了延迟参数，在这种情况下，它将直接进入运行状态
 *
 * @param pool		    从中分配延迟缓冲区的池
 * @param name		    日志标识缓冲区的可选名称
 * @param clock_rate	    每秒处理的样本数
 * @param samples_per_frame 每帧采样数
 * @param channel_count	    帧的通道数
 * @param max_delay	    要容纳的最大延迟数（毫秒），如果此值为负或小于一帧时间，则使用的默认最大延迟为400毫秒
 * @param options	    选项。如果指定了PJMEDIA_DELAY_BUF_SIMPLE_FIFO，则将使用简单的 FIFO 机制，而不是自适应实现（使用 WSOLA 扩展或收缩音频样本）
 * 						其他选项请参见 pjmedia_delay_buf_flag
 * @param p_b		    接收延迟缓冲区实例的指针
 *
 * @return		    PJ_SUCCESS if the delay buffer has been
 *			    created successfully, otherwise the appropriate
 *			    error will be returned.
 */
PJ_DECL(pj_status_t) pjmedia_delay_buf_create(pj_pool_t *pool,
					      const char *name,
					      unsigned clock_rate,
					      unsigned samples_per_frame,
					      unsigned channel_count,
					      unsigned max_delay,
					      unsigned options,
					      pjmedia_delay_buf **p_b);

/**
 * 将一帧放入缓冲区
 *
 * @param b		    延迟缓冲区
 * @param frame		要放入缓冲区的帧。此帧每帧长度必须有个样本
 *
 * @return			如果可以成功放置帧，则返回 PJ_EPENDING
 * 					如果缓冲区仍处于学习状态，则返回PJ_EPENDING。
 * 					如果帧数超过最大延迟级别，则新帧将覆盖缓冲区中最旧的帧，则返回 PJ_ETOOMANY
 */
PJ_DECL(pj_status_t) pjmedia_delay_buf_put(pjmedia_delay_buf *b,
					   pj_int16_t frame[]);

/**
 * 从缓冲区获取一帧
 *
 * @param b		    延迟缓冲区
 * @param frame		从延迟缓冲器接收帧
 *
 * @return			如果帧复制成功，则返回 PJ_SUCCESS。
 * 					如果没有可用的帧，由于缓冲区仍处于学习状态或在运行期间没有可用的缓冲区，则返回PJ_EPENDING
 * 					如果返回不成功，则该帧将填充零
 */
PJ_DECL(pj_status_t) pjmedia_delay_buf_get(pjmedia_delay_buf *b,
					   pj_int16_t frame[]);

/**
 * 重置延迟缓冲区。这将清除缓冲区的内容。但要保持学习成果
 *
 * @param b		    延迟缓冲区
 *
 * @return		    成功返回 PJ_SUCCESS，否则返回相应的错误
 */
PJ_DECL(pj_status_t) pjmedia_delay_buf_reset(pjmedia_delay_buf *b);

/**
 * 销毁延迟缓冲区
 *
 * @param b	    延迟缓冲会话
 *
 * @return	    正常返回 PJ_SUCCESS
 */
PJ_DECL(pj_status_t) pjmedia_delay_buf_destroy(pjmedia_delay_buf *b);


PJ_END_DECL

/**
 * @}
 */

#endif	/* __PJMEDIA_DELAYBUF_H__ */