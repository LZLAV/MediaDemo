/**
 * 已完成
 */
#ifndef __PJMEDIA_JBUF_H__
#define __PJMEDIA_JBUF_H__


/**
 * @file jbuf.h
 * @brief 自适应抖动缓冲实现
 */
#include <pjmedia/types.h>

/**
 * @defgroup PJMED_JBUF 自适应抖动缓冲
 * @ingroup PJMEDIA_FRAME_OP
 * @brief 自适应去抖动缓冲实现
 * @{
 *
 * 本节描述 PJMEDIA 的去抖动缓冲的实现。去抖动缓冲器可被设置为在自适应模式或固定延迟模式下操作。
 */


PJ_BEGIN_DECL


/**
 * 抖动缓冲区返回的帧类型
 */
typedef enum pjmedia_jb_frame_type 
{
    PJMEDIA_JB_MISSING_FRAME	   = 0, /**< 丢帧  */
    PJMEDIA_JB_NORMAL_FRAME	   = 1, /**< 正常帧 */
    PJMEDIA_JB_ZERO_PREFETCH_FRAME = 2, /**< 返回零帧，因为JB是缓冲中	    */
    PJMEDIA_JB_ZERO_EMPTY_FRAME	   = 3	/**< 返回零帧，因为JB为空	    */
} pjmedia_jb_frame_type;


/**
 * 抖动缓冲区丢弃算法的枚举。抖动缓冲器在任何时间连续计算抖动水平以获得最佳延迟，并且为了调整延迟，抖动缓冲器可能需要丢弃一些帧。
 */
typedef enum pjmedia_jb_discard_algo
{
    /**
     * 抖动缓冲区不应丢弃任何帧，除非抖动缓冲区已满且新帧到达时，将丢弃一帧以为新帧留出空间。
     */
    PJMEDIA_JB_DISCARD_NONE	   = 0,

    /**
     * 当延迟被认为比应该的要高得多时，在至少200毫秒内只丢弃一帧。当抖动缓冲区已满且新帧到达时，将丢弃一帧以为新帧腾出空间
     */
    PJMEDIA_JB_DISCARD_STATIC,

    /**
     * 丢弃率根据实际参数（如抖动级别和延迟）动态计算。当抖动缓冲区已满且新帧到达时，将丢弃一帧以为新帧腾出空间。
     */
    PJMEDIA_JB_DISCARD_PROGRESSIVE

} pjmedia_jb_discard_algo;


/**
 * 此结构描述抖动缓冲区状态
 */
typedef struct pjmedia_jb_state
{
    /* 设置 */
    unsigned	frame_size;	    /**< 单个帧大小，以字节为单位。  */
    unsigned	min_prefetch;	    /**< 允许的最小预取，以frms为单位。 */
    unsigned	max_prefetch;	    /**< 允许的最大预取，以frms为单位。 */

    /* 状态 */
    unsigned	burst;		    /**< 当前突发级别，以帧为单位  */
    unsigned	prefetch;	    /**< 当前预取值，以帧为单位  */
    unsigned	size;		    /**< 当前缓冲区大小，以帧为单位   */

    /* 统计的 */
    unsigned	avg_delay;	    /**< 平均延时，ms	    */
    unsigned	min_delay;	    /**< 最小延时，ms		    */
    unsigned	max_delay;	    /**< 最大延迟，ms	    */
    unsigned	dev_delay;	    /**< 延迟的标准偏差，单位为ms。*/
    unsigned	avg_burst;	    /**< 平均突发，以帧为单位。	    */
    unsigned	lost;		    /**< 丢失的帧数		    */
    unsigned	discard;	    /**< 丢弃的帧数    */
    unsigned	empty;		    /**< 获取时为空的事件数   */
} pjmedia_jb_state;


/**
 * 常量PJMEDIA_JB_DEFAULT_INIT_DELAY 指定抖动缓冲区创建期间的默认抖动缓冲区预取计数
 */
#define PJMEDIA_JB_DEFAULT_INIT_DELAY    15

/**
 * 抖动缓冲区的不透明声明
 */
typedef struct pjmedia_jbuf pjmedia_jbuf;


/**
 * 根据规范创建自适应抖动缓冲区。如果应用程序想要有一个固定的抖动缓冲区，它可以在创建抖动缓冲区后调用
 * pjmedia_jbuf_set_fixed()。另外，如果应用程序想要更改 discard 算法（PJMEDIA_JB_DISCARD_PROGRESSIVE），
 * 它可以调用 pjmedia_jbuf_set_discard()
 *
 * 此函数可能会分配大量内存以将帧保留在缓冲区中。
 *
 * @param pool		The pool to allocate memory.
 * @param name		名称以标识用于日志记录的抖动缓冲区
 * @param frame_size	将保留在抖动缓冲区中的每个帧的大小，以字节为单位。这应该与编解码器支持的最小帧大小相对应。
 * 例如，建议G.711编解码器使用 10ms帧（80字节）
 * @param max_count	抖动缓冲区中可以保留的最大帧数。这实际上意味着该抖动缓冲器可能引入的最大延迟
 * @param ptime		帧持续时间的指示，用于计算抖动重新计算之间的间隔
 * @param p_jb		接收抖动缓冲区实例的指针
 *
 * @return		成功返回 PJ_SUCCESS
 */
PJ_DECL(pj_status_t) pjmedia_jbuf_create(pj_pool_t *pool,
					 const pj_str_t *name,
					 unsigned frame_size,
					 unsigned ptime,
					 unsigned max_count,
					 pjmedia_jbuf **p_jb);

/**
 * 设置抖动缓冲区的帧持续时间
 *
 * @param jb		抖动buf
 * @param ptime		帧长
 *
 * @return		成功返回 PJ_SUCCESS
 */
PJ_DECL(pj_status_t) pjmedia_jbuf_set_ptime( pjmedia_jbuf *jb,
					     unsigned ptime);


/**
 * 将抖动缓冲设置为固定延迟模式。默认行为是根据实际的数据包延迟调整延迟。
 *
 * @param jb		抖动buf
 * @param prefetch	固定延时值，帧数
 *
 * @return		PJ_SUCCESS on success.
 */
PJ_DECL(pj_status_t) pjmedia_jbuf_set_fixed( pjmedia_jbuf *jb,
					     unsigned prefetch);


/**
 * 将抖动缓冲区设置为自适应模式
 *
 * @param jb		抖动buf
 * @param prefetch	要应用于抖动缓冲区的初始预取值。将此设置为 0 以外的值将激活预取缓冲，这是一种抖动缓冲功能，每次它变空时，
 * 它都不会返回正常帧，直到其大小达到此处指定的数字
 * @param min_prefetch	必须应用于每个传入数据包的最小延迟，以帧数为单位
 * @param max_prefetch	预取延迟的最大允许值，帧数
 *
 * @return		PJ_SUCCESS on success.
 */
PJ_DECL(pj_status_t) pjmedia_jbuf_set_adaptive( pjmedia_jbuf *jb,
					        unsigned prefetch,
					        unsigned min_prefetch,
						unsigned max_prefetch);


/**
 * 设置抖动缓冲丢弃算法。在抖动缓冲区创建中设置的默认丢弃算法是PJMEDIA_JB_DISCARD_PROGRESSIVE
 *
 * @param jb		抖动buf
 * @param algo		使用的丢弃算法
 *
 * @return		PJ_SUCCESS on success.
 */
PJ_DECL(pj_status_t) pjmedia_jbuf_set_discard(pjmedia_jbuf *jb,
					      pjmedia_jb_discard_algo algo);


/**
 * 销毁抖动buf 实例
 *
 * @param jb		抖动buf
 *
 * @return		PJ_SUCCESS on success.
 */
PJ_DECL(pj_status_t) pjmedia_jbuf_destroy(pjmedia_jbuf *jb);


/**
 * 重新启动抖动。此函数用于刷新缓冲区中的所有数据包并重置内部序列号
 *
 * @param jb		抖动buf
 *
 * @return		PJ_SUCCESS on success.
 */
PJ_DECL(pj_status_t) pjmedia_jbuf_reset(pjmedia_jbuf *jb);

/**
 * 将一帧放入抖动缓冲区。如果可以接受帧（基于序列号），抖动缓冲器将复制帧并将其放在缓冲器中的适当位置
 *
 * 当多个线程同时访问抖动缓冲区时，应用程序必须管理自己的同步
 *
 * @param jb		抖动buf
 * @param frame		指向要存储在抖动缓冲区中的帧缓冲区的指针
 * @param size		帧大小
 * @param frame_seq	帧序列号
 */
PJ_DECL(void) pjmedia_jbuf_put_frame( pjmedia_jbuf *jb, 
				      const void *frame, 
				      pj_size_t size, 
				      int frame_seq);

/**
 * 将一帧放入抖动缓冲区。如果可以接受帧（基于序列号），抖动缓冲器将复制帧并将其放在缓冲器中的适当位置。
 *
 * 当多个线程同时访问抖动缓冲区时，应用程序必须管理自己的同步
 *
 * @param jb		抖动buf
 * @param frame		指向要存储在抖动缓冲区中的帧缓冲区的指针
 * @param size		帧大小
 * @param bit_info	帧的位精确信息，例如：帧可能不完全在 8字节边界处开始和结束，因此此字段可用于指定开始和结束位偏移
 * @param frame_seq	帧的序列号
 * @param discarded	标记该帧是否被抖动buf丢弃
 */
PJ_DECL(void) pjmedia_jbuf_put_frame2( pjmedia_jbuf *jb, 
				       const void *frame, 
				       pj_size_t size, 
				       pj_uint32_t bit_info,
				       int frame_seq,
				       pj_bool_t *discarded);

/**
 * 将一帧放入抖动缓冲区。如果可以接受帧（基于序列号），抖动缓冲器将复制帧并将其放在缓冲器中的适当位置。
 *
 * 当多个线程同时访问抖动缓冲区时，应用程序必须管理自己的同步。
 *
 * @param jb		抖动buf
 * @param frame		指向要存储在抖动缓冲区中的帧缓冲区的指针
 * @param size		帧大小
 * @param bit_info	帧的位精确信息，例如：帧可能不能在 8进制边界处精确地开始和结束，因此该字段可用于指定起始位和结束位偏移
 * @param frame_seq	帧的序列号
 * @param frame_ts	帧的时间戳
 * @param discarded	标记该帧是否被抖动buf丢弃
 */
PJ_DECL(void) pjmedia_jbuf_put_frame3( pjmedia_jbuf *jb, 
				       const void *frame, 
				       pj_size_t size, 
				       pj_uint32_t bit_info,
				       int frame_seq,
				       pj_uint32_t frame_ts,
				       pj_bool_t *discarded);
/**
 * 从抖动缓冲区获取一帧。抖动缓冲区将返回最早的帧从它的缓冲区，当它是可用的。
 *
 * 当多个线程同时访问抖动缓冲区时，应用程序必须管理自己的同步。
 *
 * @param jb		抖动buf
 * @param frame		从抖动缓冲区接收有效负载。应用程序必须确保缓冲区具有适当的大小（即不小于创建抖动缓冲区时指定的帧大小）
 * 当此函数返回的帧类型为 PJMEDIA_JB_NORMAL_FRAME 时，抖动缓冲区仅将一个帧复制到此缓冲区
 *
 * @param p_frm_type	指向接收帧类型的指针。如果jitter buffer当前为空或 bufferring，则帧类型将设置为PJMEDIA_JB_ZERO_FRAME，
 * 并且不会复制任何帧。如果抖动缓冲区检测到当前序列号的帧丢失，帧类型将设置为PJMEDIA_JB_MISSING_FRAME，并且不会复制任何帧。如果有帧，
 * 抖动缓冲区会将帧复制到缓冲区，并且帧类型会设置为PJMEDIA_JB_NORMAL_FRAME
 *
 */
PJ_DECL(void) pjmedia_jbuf_get_frame( pjmedia_jbuf *jb, 
				      void *frame, 
				      char *p_frm_type);

/**
 * 从抖动缓冲区获取一帧。抖动缓冲区将返回最早的帧从它的缓冲区，当它是可用的
 *
 * @param jb		抖动buf
 * @param frame		从抖动缓冲区接收有效负载。
 * 					@请参见pjmedia_jbuf_get_frame()
 * @param size		输入时，它指向最大缓冲区长度。
 * 					输出时，它将填充接收到的帧大小
 * @param p_frm_type	指向接收帧类型的指针。
 * 						@请参见pjmedia_jbuf_get_frame()
 * @param bit_info	帧的位精确信息，例如：帧可能不完全在 8 位字节边界处开始和结束，因此此字段可用于指定开始和结束位偏移
 */
PJ_DECL(void) pjmedia_jbuf_get_frame2(pjmedia_jbuf *jb, 
				      void *frame, 
				      pj_size_t *size, 
				      char *p_frm_type,
				      pj_uint32_t *bit_info);


/**
 * 从抖动缓冲区获取一帧。抖动缓冲区将返回最早的帧从它的缓冲区，当它是可用的。
 *
 * @param jb		抖动buf
 * @param frame		从抖动缓冲区接收有效负载。
 * 					@请参见pjmedia_jbuf_get_frame()
 * @param size		输入时，它指向最大缓冲区长度
 * 					输出时，它将填充接收到的帧大小
 * @param p_frm_type	指向接收帧类型的指针
 *						@参见 pjmedia_jbuf_get_frame().
 * @param bit_info	帧的位精确信息，例如：帧可能不完全在 8位字节边界处开始和结束，因此此字段可用于指定开始和结束位偏移
 * @param ts		帧的时间戳
 * @param seq		帧的序列号
 */
PJ_DECL(void) pjmedia_jbuf_get_frame3(pjmedia_jbuf *jb, 
				      void *frame, 
				      pj_size_t *size, 
				      char *p_frm_type,
				      pj_uint32_t *bit_info,
				      pj_uint32_t *ts,
				      int *seq);


/**
 * 从抖动缓冲区 peek一帧。不会修改抖动缓冲区状态
 *
 * @param jb		抖动buf
 * @param offset	从要偷看的最旧帧偏移
 * @param frame		从抖动缓冲区接收有效负载。
 * 					@请参见pjmedia_jbuf_get_frame()
 * @param size		接收帧大小的指针
 * @param p_frm_type	指向接收帧类型的指针。
 * 						@请参见pjmedia_jbuf_get_frame()
 * @param bit_info	帧的位精确信息，例如：帧可能不完全在8位字节边界处开始和结束，因此此字段可用于指定开始和结束位偏移
 * @param ts		帧时间戳
 * @param seq		帧序列号
 */
PJ_DECL(void) pjmedia_jbuf_peek_frame(pjmedia_jbuf *jb,
				      unsigned offset,
				      const void **frame, 
				      pj_size_t *size, 
				      char *p_frm_type,
				      pj_uint32_t *bit_info,
				      pj_uint32_t *ts,
				      int *seq);


/**
 * 从抖动缓冲区中删除帧
 *
 * @param jb		抖动buf
 * @param frame_cnt	要删除的帧数
 *
 * @return		已成功删除的帧数
 */
PJ_DECL(unsigned) pjmedia_jbuf_remove_frame(pjmedia_jbuf *jb, 
					    unsigned frame_cnt);

/**
 * 检查抖动缓冲区是否已满
 *
 * @param jb		抖动buf
 *
 * @return		满了返回 PJ_TRUE
 */
PJ_DECL(pj_bool_t) pjmedia_jbuf_is_full(const pjmedia_jbuf *jb);


/**
 * 获取抖动缓冲区当前状态/设置
 *
 * @param jb		抖动buf
 * @param state		接收抖动缓冲状态的缓冲区
 *
 * @return		成功返回 PJ_SUCCESS
 */
PJ_DECL(pj_status_t) pjmedia_jbuf_get_state( const pjmedia_jbuf *jb,
					     pjmedia_jb_state *state );



PJ_END_DECL

/**
 * @}
 */

#endif	/* __PJMEDIA_JBUF_H__ */
