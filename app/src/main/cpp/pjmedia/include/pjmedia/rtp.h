/**
 * 已完成
 */
#ifndef __PJMEDIA_RTP_H__
#define __PJMEDIA_RTP_H__


/**
 * @file rtp.h
 * @brief RTP包和RTP会话声明
 */
#include <pjmedia/types.h>


PJ_BEGIN_DECL


/**
 * @defgroup PJMED_RTP RTP会话和封装（RFC 3550）
 * @ingroup PJMEDIA_SESSION
 * @brief RTP格式与会话管理
 * @{
 *
 * RTP模块被设计成只依赖于 PJLIB，它不依赖于PJMEDIA库的任何其他部分。RTP模块甚至不依赖于任何传输（套接字），
 * 以促进更多的使用，例如在DSP开发中（传输可以由不同的处理器处理）
 *
 * RTCP实现在单独的模块中可用。请参阅@ref PJMED_RTCP
 *
 * 本模块提供的功能包括：
 * 		-为每个传出数据包创建RTP报头
 * 		-将RTP包解码成RTP报头和有效载荷
 * 		-提供简单的RTP会话管理（序列号等）
 *
 * RTP模块根本不使用任何动态内存。
 *
 * \section P1 如何使用RTP模块
 * 
 * 第一个应用程序必须调用 pjmedia_rtp_session_init() 来初始化rtp会话
 *
 * 当应用程序想要发送RTP数据包时，它需要调用 pjmedia_rtp_encode_rtp() 来构建RTP报头。注意，这不会构建完整的RTP包，
 * 而是只构建报头。然后，应用程序可以将报头与有效负载连接起来，或者使用scatter-gather传输API（例如.sendv()）发送两个片段（报头和有效负载）
 *
 * 当应用程序收到一个RTP数据包时，首先它应该调用 pjmedia_rtp_decode_rtp 来解码RTP报头和负载，然后它应该调用
 * pjmedia_rtp_session_update 来检查我们是否可以处理RTP负载，并让RTP会话更新其内部状态。无论RTP包中存在任何选项，解码功能都保证将有效负载指向正确的位置。
 *
 */

#ifdef _MSC_VER
#   pragma warning(disable:4214)    // bit field types other than int
#endif


/**
 * RTP数据包头。请注意，网络字节顺序
 */
#pragma pack(1)
struct pjmedia_rtp_hdr
{
#if defined(PJ_IS_BIG_ENDIAN) && (PJ_IS_BIG_ENDIAN!=0)
    pj_uint16_t v:2;		/**< 包类型/版本	    */
    pj_uint16_t p:1;		/**< 填充标识		    */
    pj_uint16_t x:1;		/**< 扩展标志		    */
    pj_uint16_t cc:4;		/**< CSRC 数			    */
    pj_uint16_t m:1;		/**< 标记位		    */
    pj_uint16_t pt:7;		/**< 负载类型		    */
#else
    pj_uint16_t cc:4;		/**< CSRC 数		    */
    pj_uint16_t x:1;		/**< 扩展标志	    */
    pj_uint16_t p:1;		/**< 填充标识	    */
    pj_uint16_t v:2;		/**< 包类型/版本	    */
    pj_uint16_t pt:7;		/**< 负载类型		    */
    pj_uint16_t m:1;		/**< 标记位		    */
#endif
    pj_uint16_t seq;		/**< 序列号		    */
    pj_uint32_t ts;		/**< 时间戳			    */
    pj_uint32_t ssrc;		/**< 同步源	    */
};
#pragma pack()

/**
 * @see pjmedia_rtp_hdr
 */
typedef struct pjmedia_rtp_hdr pjmedia_rtp_hdr;


/**
 * RTP 扩展头
 */
struct pjmedia_rtp_ext_hdr
{
    pj_uint16_t	profile_data;	/**< 配置文件数据    */
    pj_uint16_t	length;		/**< 长度	    */
};

/**
 * @see pjmedia_rtp_ext_hdr
 */
typedef struct pjmedia_rtp_ext_hdr pjmedia_rtp_ext_hdr;

/**
 * 这将包含RTP头解码输出
 */
struct pjmedia_rtp_dec_hdr
{
    /* RTP扩展头输出解码 */
    pjmedia_rtp_ext_hdr *ext_hdr;
    pj_uint32_t *ext;
    unsigned ext_len;
};

/**
 * @see pjmedia_rtp_dec_hdr
 */
typedef struct pjmedia_rtp_dec_hdr pjmedia_rtp_dec_hdr;

#pragma pack(1)

/**
 * DTMF电话事件声明（RFC2833）。
 */
struct pjmedia_rtp_dtmf_event
{
    pj_uint8_t	event;	    /**< 事件类型ID	    */
    pj_uint8_t	e_vol;	    /**< 事件量	    */
    pj_uint16_t	duration;   /**< 事件持续时间   */
};

/**
 * @see pjmedia_rtp_dtmf_event
 */
typedef struct pjmedia_rtp_dtmf_event pjmedia_rtp_dtmf_event;

#pragma pack()


/**
 * 一种通用的序列号管理，由RTP和RTCP使用
 */
struct pjmedia_rtp_seq_session
{
    pj_uint16_t	    max_seq;	    /**< 最高序列号    */
    pj_uint32_t	    cycles;	    /**< 序列号循环的移位计数 */
    pj_uint32_t	    base_seq;	    /**< 基本序列号		    */
    pj_uint32_t	    bad_seq;        /**< 最后一个“坏”序号+1    */
    pj_uint32_t	    probation;      /**< sequ,直到源有效 */
};

/**
 * @see pjmedia_rtp_seq_session
 */
typedef struct pjmedia_rtp_seq_session pjmedia_rtp_seq_session;


/**
 * RTP 会话描述
 */
struct pjmedia_rtp_session
{
    pjmedia_rtp_hdr	    out_hdr;    /**< 已保存传出数据包的 hdr  */
    pjmedia_rtp_seq_session seq_ctrl;   /**< 序列号管理  */
    pj_uint16_t		    out_pt;	/**< 默认传出负载类型 */
    pj_uint32_t		    out_extseq; /**< 传出扩展序列#	    */
    pj_bool_t		    has_peer_ssrc;/**< 是否存在对等 SSRC	    */
    pj_uint32_t		    peer_ssrc;  /**< 对等 SSRC.			    */
    pj_uint32_t		    received;   /**< 接收的数据包数    */
};

/**
 * @see pjmedia_rtp_session
 */
typedef struct pjmedia_rtp_session pjmedia_rtp_session;


/**
 * 此结构用于接收有关传入RTP数据包状态的附加信息。
 */
struct pjmedia_rtp_status
{
    union {
	struct flag {
	    int	bad:1;	    /**< 通用标志，指示序列错误，应用程序不应处理此数据包。更多信息将在其他标志中给出 */
	    int badpt:1;    /**< 无效负载类型。  */
	    int badssrc:1;  /**< 无效 SSRC				    */
	    int	dup:1;	    /**< 表示重复的数据包	    */
	    int	outorder:1; /**< 表示数据包出现故障		    */
	    int	probation:1;/**< 指示会话处于试用状态，直到收到更多数据包。	    */
	    int	restart:1;  /**< 表示序列号已大幅度跳转，并且内部基序列号已调整	    */
	} flag;		    /**< 状态标志				    */

	pj_uint16_t value;  /**< 状态值，方便地处理所有标志 */

    } status;		    /**< 状态信息联合体	    */

    pj_uint16_t	diff;	    /**< 与前一个数据包的序列号不同。通常该值应为1。值大于1可能表示数据包丢失。如果接收到序列较低的数据包，则该值将设置为zero.
 * 							如果基序列已重新启动，值将为1 */
};


/**
 * RTP 会话设置
 */
typedef struct pjmedia_rtp_session_setting
{
    pj_uint8_t	     flags;	    /**< 位掩码标志，用于指定是否设置此类字段。位掩码内容包括：
 * 									（位0为LSB）
 * 									bit#0：默认有效负载类型
 * 									位1：发送器SSRC
 * 									第2位：序列
 * 									位3：时间戳
 * 									第4位：对等SSRC */
    int		     default_pt;    /**< 默认负载类型		    */
    pj_uint32_t	     sender_ssrc;   /**< 发送SSRC		    */
    pj_uint32_t	     peer_ssrc;     /**< 对等SSRC		    */
    pj_uint16_t	     seq;	    /**< 序列号			    */
    pj_uint32_t	     ts;	    /**< 时间戳			    */
} pjmedia_rtp_session_setting;


/**
 * @see pjmedia_rtp_status
 */
typedef struct pjmedia_rtp_status pjmedia_rtp_status;


/**
 * 此函数将根据给定的参数初始化RTP会话
 *
 * @param ses		会话
 * @param default_pt	默认的负载类型
 * @param sender_ssrc	用于传出数据包的SSRC，按主机字节顺序
 *
 * @return		PJ_SUCCESS if successfull.
 */
PJ_DECL(pj_status_t) pjmedia_rtp_session_init( pjmedia_rtp_session *ses,
					       int default_pt, 
					       pj_uint32_t sender_ssrc );

/**
 * 此函数将根据RTP会话设置中定义的给定参数初始化RTP会话
 *
 * @param ses		会话
 * @param settings	会话设置
 *
 * @return		成功返回 PJ_SUCCESS
 */
PJ_DECL(pj_status_t) pjmedia_rtp_session_init2( 
				    pjmedia_rtp_session *ses,
				    pjmedia_rtp_session_setting settings);


/**
 * 基于参数和RTP会话的当前状态创建RTP头
 *
 * @param ses		会话
 * @param pt		负载类型
 * @param m		标识位
 * @param payload_len	有效负载长度（字节）
 * @param ts_len	时间戳长度。
 * @param rtphdr	返回时将指向RTP包头
 * @param hdrlen	返回时将指示RTP数据包头的大小
 *
 * @return		PJ_SUCCESS if successfull.
 */
PJ_DECL(pj_status_t) pjmedia_rtp_encode_rtp( pjmedia_rtp_session *ses, 
					     int pt, int m,
					     int payload_len, int ts_len,
					     const void **rtphdr, 
					     int *hdrlen );

/**
 * 此函数将传入的数据包解码为 RTP报头和有效负载。
 * 无论RTP包中存在任何选项，解码功能都保证将有效负载指向正确的位置
 *
 * 请注意，此函数不会修改返回的RTP头到主机字节顺序
 *
 * @param ses		会话
 * @param pkt		RTP 数据包
 * @param pkt_len	rtp 数据包长度
 * @param hdr		返回时将指向数据包内的 RTP报头的位置。请注意，RTP头将按原样返回，这意味着字段将按网络字节顺序排列
 * @param payload	返回时将指向数据包内有效负载的位置
 * @param payloadlen	返回时将指示有效负载的大小
 *
 * @return		PJ_SUCCESS if successfull.
 */
PJ_DECL(pj_status_t) pjmedia_rtp_decode_rtp( pjmedia_rtp_session *ses, 
					     const void *pkt, int pkt_len,
					     const pjmedia_rtp_hdr **hdr,
					     const void **payload,
					     unsigned *payloadlen);


/**
 * 此函数将传入的数据包解码为RTP报头和有效负载。无论RTP包中存在任何选项，解码功能都保证将有效负载指向正确的位置。
 *
 * 请注意，此函数不会修改返回的RTP头到主机字节顺序。
 *
 * @param ses		会话
 * @param pkt		RTP 数据包
 * @param pkt_len	rtp 数据包长度
 * @param hdr		返回时将指向数据包内的 RTP报头的位置。请注意，RTP头将按原样返回，这意味着字段将按网络字节顺序排列
 * @param dec_hdr	返回时将指向数据包内附加RTP报头的位置（如果有的话）
 * @param payload	返回时将指向数据包内有效负载的位置。
 * @param payloadlen	返回时将指示有效负载的大小
 *
 * @return		成功返回 PJ_SUCCESS
 */
PJ_DECL(pj_status_t) pjmedia_rtp_decode_rtp2(
				    pjmedia_rtp_session *ses,
				    const void *pkt, int pkt_len,
				    const pjmedia_rtp_hdr **hdr,
				    pjmedia_rtp_dec_hdr *dec_hdr,
				    const void **payload,
				    unsigned *payloadlen);

/**
 * 每次接收到RTP数据包时调用此函数，检查是否可以接收该数据包，并让RTP会话执行其内部计算。
 *
 * @param ses	    会话
 * @param hdr	    传入数据包的RTP头。网络字节顺序
 * @param seq_st    可选结构接收RTP数据包处理的状态
 */
PJ_DECL(void) pjmedia_rtp_session_update( pjmedia_rtp_session *ses, 
					  const pjmedia_rtp_hdr *hdr,
					  pjmedia_rtp_status *seq_st);


/**
 * 每次接收到RTP数据包时调用此函数，检查是否可以接收该数据包，并让RTP会话执行其内部计算
 *
 * @param ses	    会话
 * @param hdr	    传入数据包的RTP头。网络字节顺序
 * @param seq_st    可选结构接收RTP数据包处理的状态
 * @param check_pt  指示是否需要验证有效负载类型的标志
 *
 * @see pjmedia_rtp_session_update()
 */
PJ_DECL(void) pjmedia_rtp_session_update2(pjmedia_rtp_session *ses, 
					  const pjmedia_rtp_hdr *hdr,
					  pjmedia_rtp_status *seq_st,
					  pj_bool_t check_pt);


/*
 * INTERNAL:
 */

/** 
 * 用于创建序列号控件的内部函数，由RTCP实现共享
 *
 * @param seq_ctrl  序列控件实例
 * @param seq	    要初始化的序列号
 */
void pjmedia_rtp_seq_init(pjmedia_rtp_seq_session *seq_ctrl, 
			  pj_uint16_t seq);


/** 
 * 内部函数更新序列控制，由RTCP实现共享
 *
 * @param seq_ctrl	序列控件实例
 * @param seq		要更新的序列号
 * @param seq_status	用于接收有关数据包的附加信息的可选结构
 */
void pjmedia_rtp_seq_update( pjmedia_rtp_seq_session *seq_ctrl, 
			     pj_uint16_t seq,
			     pjmedia_rtp_status *seq_status);

/**
 * @}
 */

PJ_END_DECL


#endif	/* __PJMEDIA_RTP_H__ */
