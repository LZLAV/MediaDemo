/* $Id: conference.h 5792 2018-05-15 08:23:44Z ming $ */
/* 
 * Copyright (C) 2008-2011 Teluu Inc. (http://www.teluu.com)
 * Copyright (C) 2003-2008 Benny Prijono <benny@prijono.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */
#ifndef __PJMEDIA_CONF_H__
#define __PJMEDIA_CONF_H__


/**
 * @file conference.h
 * @brief 会议桥
 */
#include <pjmedia/port.h>

/**
 * @defgroup PJMEDIA_CONF 会议桥
 * @ingroup PJMEDIA_PORT
 * @brief 音频会议桥的实现
 * @{
 *
 * 本文描述了PJMEDIA中的会议桥实现。会议桥提供了强大而非常有效的机制来路由音频流，并在需要时混合音频信号。
 *
 * 有关会议桥打开时媒体流的更多信息
 * 使用的描述 见 http://www.pjsip.org/trac/wiki/media-flow
 */

PJ_BEGIN_DECL

/**
 * pjmedia_port_info中的会议桥签名
 */
#define PJMEDIA_CONF_BRIDGE_SIGNATURE	PJMEDIA_SIG_PORT_CONF

/**
 * pjmedia_port_info 中的音频交换机签名
 */
#define PJMEDIA_CONF_SWITCH_SIGNATURE	PJMEDIA_SIG_PORT_CONF_SWITCH


/**
 * 会议桥的不透明类型
 */
typedef struct pjmedia_conf pjmedia_conf;

/**
 * 会议端口信息
 */
typedef struct pjmedia_conf_port_info
{
    unsigned		slot;		    /**< Slot number 插槽编号   */
    pj_str_t		name;		    /**< 端口名称	    */
    pjmedia_format	format;		    /**< 格式		    */
    pjmedia_port_op	tx_setting;	    /**< 传输设置    */
    pjmedia_port_op	rx_setting;	    /**< 接收设置    */
    unsigned		listener_cnt;	    /**< 侦听器数    */
    unsigned	       *listener_slots;	    /**< 侦听器数组    */
    unsigned	       *listener_adj_level; /**< 侦听器级别调整数组 */
    unsigned		transmitter_cnt;    /**< 变送器数量。Number of transmitter.	    */
    unsigned		clock_rate;	    /**< 端口的时钟速率   */
    unsigned		channel_count;	    /**< 通道数	    */
    unsigned		samples_per_frame;  /**< 每帧采样数	    */
    unsigned		bits_per_sample;    /**< 每样本位数	    */
    int			tx_adj_level;	    /**< Tx 水平调整    */
    int			rx_adj_level;	    /**< Rx 水平调整  */
} pjmedia_conf_port_info;


/**
 * 会议端口选项。这里的值可以在创建会议桥时指定的位掩码中组合。
 */
enum pjmedia_conf_option
{
    PJMEDIA_CONF_NO_MIC  = 1,	/**< 禁用麦克风设备的音频流		    */
    PJMEDIA_CONF_NO_DEVICE = 2,	/**< 不要创建声音设备	    */
    PJMEDIA_CONF_SMALL_FILTER=4,/**< 重采样时使用SMALL 滤波器*/
    PJMEDIA_CONF_USE_LINEAR=8	/**< 使用线性重采样而不是基于滤波器    */
};


/**
 * 使用指定的参数创建会议桥。采样率、samples per frame和bits per sample将用于桥接器的内部操作（例如，混合音频帧时）。
 * 但是，具有不同配置的端口可以连接到网桥。在这种情况下，桥接器能够执行采样率转换，并且在每帧采样数不同的情况下进行缓冲。
 *
 * 对于此版本的PJMEDIA，每个示例仅支持16位。
 * 对于此版本的PJMEDIA，端口的通道计数必须与网桥的通道计数匹配。
 *
 * 在正常操作下（即未指定 PJMEDIA_CONF_NO_DEVICE 选项），网桥在内部创建声音设备的实例，并将声音设备连接到网桥的端口 0。
 *
 * 如果指定了PJMEDIA_CONF_NO_DEVICE options，则不会在会议桥中创建声音设备。应用程序必须通过调用 pjmedia_conf_get_master_port()，
 * 获取网桥的端口接口，并通过调用 pjmedia_snd_port_connect()，将此端口接口连接到声音设备端口，或者如果应用程序不想实例化任何声音设备，
 * 则连接到主端口（pjmedia_master_port）
 *
 * 声音设备或主端口对网桥的运行至关重要，因为它为网桥提供了周期性处理音频帧所需的时钟。在内部，当 port 0 调用get_frame()时，桥运行。
 *
 * @param pool		    用于为声音设备分配桥接器和附加缓冲区的池
 * @param max_slots	    要在网桥中创建的最大插槽/端口数。请注意，网桥内部为声音设备使用一个端口，因此实际最大端口数将小于此值
 * @param sampling_rate	    设置桥的采样率。此值还用于设置声音设备的采样率
 * @param channel_count	    PCM 流中的通道数目。通常情况下，单声道的值为 1，但应用程序可能会指定立体声的值为 2。请注意，将连接到
 * 							网桥的所有端口的通道数必须与网桥相同
 * @param samples_per_frame 设置每帧的采样数。此值也用于设置声音设备
 * @param bits_per_sample   设置每个样本的位数。此值也用于设置声音设备。目前每个样本仅支持16位
 * @param options	    要为网桥设置的位掩码选项。这些选项是由 pjmedia_conf_option 枚举构造的
 * @param p_conf	    接收会议桥实例的指针
 *
 * @return		    会议桥成功创建则返回 PJ_SUCCESS
 */
PJ_DECL(pj_status_t) pjmedia_conf_create( pj_pool_t *pool,
					  unsigned max_slots,
					  unsigned sampling_rate,
					  unsigned channel_count,
					  unsigned samples_per_frame,
					  unsigned bits_per_sample,
					  unsigned options,
					  pjmedia_conf **p_conf );


/**
 * 销毁会议桥
 *
 * @param conf		   会议桥
 *
 * @return		    PJ_SUCCESS on success.
 */
PJ_DECL(pj_status_t) pjmedia_conf_destroy( pjmedia_conf *conf );


/**
 * 获取会议桥的主端口接口。主端口对应于网桥的端口 0。这只在应用程序希望自己管理声音设备时才有用，而不是允许桥接器隐式地自动创建声音设备。
 *
 * 如果在创建网桥时指定了 PJMEDIA_CONF_NO_DEVICE 选项，则此函数仅返回端口接口
 *
 * 应用程序可以通过调用 pjmedia_snd_port_connect() 将此函数返回的端口连接到声音设备
 *
 * @param conf		    会议桥
 *
 * @return		    只有在创建桥时指定PJMEDIA_CONF_NO_DEVICE options时，桥的端口 0的端口接口
 */
PJ_DECL(pjmedia_port*) pjmedia_conf_get_master_port(pjmedia_conf *conf);


/**
 * 设置主端口名称
 *
 * @param conf		    会议桥
 * @param name		    要分配的名称。
 *
 * @return		    PJ_SUCCESS on success.
 */
PJ_DECL(pj_status_t) pjmedia_conf_set_port0_name(pjmedia_conf *conf,
						 const pj_str_t *name);


/**
 * 将媒体端口添加到会议桥
 *
 * 默认情况下，新的会议端口将同时启用 TX和 RX，但它不会连接到任何其他端口。应用程序应该调用 pjmedia_conf_connect_port()，以启用与此端口之间的音频传输和接收
 *
 * 一旦媒体端口连接到网桥中的其他端口，网桥将继续调用 get_frame() 并将 put_frame() 放入该端口，从而允许媒体在端口之间流动
 *
 * @param conf		会议桥
 * @param pool		池来为此端口分配缓冲区
 * @param strm_port	流端口接口
 * @param name		端口的可选名称。如果此值为空，则名称将取自端口信息中的名称
 * @param p_slot	指针，用于接收会议桥中端口的插槽索引
 *
 * @return		PJ_SUCCESS on success.
 */
PJ_DECL(pj_status_t) pjmedia_conf_add_port( pjmedia_conf *conf,
					    pj_pool_t *pool,
					    pjmedia_port *strm_port,
					    const pj_str_t *name,
					    unsigned *p_slot );


/**
 * <b>警告：
 * 	此API从1.3版开始就被弃用，并将在以后的版本中删除，请改用@ref PJMEDIA_SPLITCOMB
 *
 * 	创建被动媒体端口并将其添加到会议桥。与使用 pjmedia_conf_add_port() 添加的“普通”媒体端口不同，使用此函数创建的
 * 	媒体端口不会由桥调用其 get_frame()和 put_frame()；相反，应用程序必须不断地将这些函数调用到端口，以允许媒体 from/to 向端口
 *
 * 	返回此函数后，应用程序将获得两个对象：
 * 		网桥中端口的插槽号，以及指向媒体端口的指针，应用程序必须在其中开始调用 get_frame()并将 put_frame()到指定端口
 *
 * @param conf		    会议桥
 * @param pool		    池来为此端口分配缓冲区等
 * @param name		    要分配给端口的名称
 * @param clock_rate	    时钟速率/采样速率
 * @param channel_count	    通道数
 * @param samples_per_frame 每帧采样数
 * @param bits_per_sample   每个样本的位数
 * @param options	    选项（此时应为零）
 * @param p_slot	    用于接收会议桥中端口的插槽索引的指针
 * @param p_port	    接收端口实例的指针
 *
 * @return		    PJ_SUCCESS on success, or the appropriate error 
 *			    code.
 */
PJ_DECL(pj_status_t) pjmedia_conf_add_passive_port( pjmedia_conf *conf,
						    pj_pool_t *pool,
						    const pj_str_t *name,
						    unsigned clock_rate,
						    unsigned channel_count,
						    unsigned samples_per_frame,
						    unsigned bits_per_sample,
						    unsigned options,
						    unsigned *p_slot,
						    pjmedia_port **p_port );


/**
 * 更改端口的TX和RX设置
 *
 * @param conf		会议桥
 * @param slot		会议网桥中的端口号/插槽
 * @param tx		传输到此端口的设置
 * @param rx		从此端口接收的设置
 *
 * @return		PJ_SUCCESS on success.
 */
PJ_DECL(pj_status_t) pjmedia_conf_configure_port( pjmedia_conf *conf,
						  unsigned slot,
						  pjmedia_port_op tx,
						  pjmedia_port_op rx);


/**
 * 启用从指定源插槽到指定接收器插槽的单向音频。
 * 应用程序可以调整电平，使从源插槽传输到接收器插槽的信号更响亮或更安静。计算公式如下：
 * 		输出=输入*（adj_level+128）/128。使用这个，零表示没有调整，值 -128将使信号静音，+128将使信号变大 100%，+256将使信号变大 200%，等等。
 *
 * 	电平调整仅适用于特定连接（即仅适用于从源到接收器的信号），而 pjmedia_conf_adjust_tx_level()/pjmedia_conf_adjust_rx_level() 适用于 from/to
 * 	该端口的所有信号。信号调整将按以下顺序累加：
 * 		来自源的信号将按照pjmedia_conf_adjust_rx_level(), 中指定的电平进行调整，然后按照通过此API指定的电平进行调整，最后按照指定给接收器
 * 		pjmedia_conf_adjust_tx_level() 的电平进行调整。
 *
 * @param conf		会议桥
 * @param src_slot	源
 * @param sink_slot	汇
 * @param adj_level	调整级别，必须大于或等于-128。值为零表示不需要进行电平调整，值-128将使信号静音，+128将使信号变大100%，+256将使信号变大200%等等.
 * 				See 公式的函数说明
 *
 * @return		PJ_SUCCES on success.
 */
PJ_DECL(pj_status_t) pjmedia_conf_connect_port( pjmedia_conf *conf,
						unsigned src_slot,
						unsigned sink_slot,
						int adj_level );


/**
 * 断开从指定源到指定接收器插槽的单向音频连接。
 *
 * @param conf		会议桥
 * @param src_slot	源
 * @param sink_slot	汇
 *
 * @return		PJ_SUCCESS on success.
 */
PJ_DECL(pj_status_t) pjmedia_conf_disconnect_port( pjmedia_conf *conf,
						   unsigned src_slot,
						   unsigned sink_slot );


/**
 * 获取当前注册到会议桥的端口数
 *
 * @param conf		会议桥
 *
 * @return		当前注册到会议网桥的端口数
 */
PJ_DECL(unsigned) pjmedia_conf_get_port_count(pjmedia_conf *conf);


/**
 * 获取当前在网桥中设置的端口连接总数
 * 
 * @param conf		会议桥
 *
 * @return		PJ_SUCCESS on success.
 */
PJ_DECL(unsigned) pjmedia_conf_get_connect_count(pjmedia_conf *conf);


/**
 * 从会议网桥中删除指定的端口
 *
 * @param conf		会议桥
 * @param slot		要删除的端口索引
 *
 * @return		PJ_SUCCESS on success.
 */
PJ_DECL(pj_status_t) pjmedia_conf_remove_port( pjmedia_conf *conf,
					       unsigned slot );



/**
 * 列举桥上被占用的港口
 *
 * @param conf		会议桥
 * @param ports		要填写的端口号数组
 * @param count		在输入时，指定阵列中的最大端口数。返回时，它将填充实际的端口数
 *
 * @return		PJ_SUCCESS on success.
 */
PJ_DECL(pj_status_t) pjmedia_conf_enum_ports( pjmedia_conf *conf,
					      unsigned ports[],
					      unsigned *count );


/**
 * 获取端口信息
 *
 * @param conf		会议桥
 * @param slot		端口索引
 * @param info		接收信息的指针
 *
 * @return		PJ_SUCCESS on success.
 */
PJ_DECL(pj_status_t) pjmedia_conf_get_port_info( pjmedia_conf *conf,
						 unsigned slot,
						 pjmedia_conf_port_info *info);


/**
 * 获取占用端口信息
 *
 * @param conf		会议桥
 * @param size		输入时，包含要检索的最大信息数。在输出时，包含已复制的实际信息数
 * @param info		信息数组
 *
 * @return		PJ_SUCCESS on success.
 */
PJ_DECL(pj_status_t) pjmedia_conf_get_ports_info(pjmedia_conf *conf,
						 unsigned *size,
						 pjmedia_conf_port_info info[]
						 );


/**
 * 获取发送到指定端口或从指定端口接收到的最后一个信号电平
 * 这将检索指定端口发送或接收的音频的“实时”信号电平。应用程序可以周期性地调用此函数以向VU表显示信号电平。
 *
 * 信号电平为0到255之间的整数值，0表示无信号，255表示最大信号电平
 *
 * @param conf		会议桥
 * @param slot		Slot number.
 * @param tx_level	可选参数，用于接收传输到指定端口的信号电平（即从网桥到端口的方向）
 * @param rx_level	用于接收从端口接收的信号电平的可选参数（即，方向是从端口到网桥）
 *
 * @return		PJ_SUCCESS on success.
 */
PJ_DECL(pj_status_t) pjmedia_conf_get_signal_level(pjmedia_conf *conf,
						   unsigned slot,
						   unsigned *tx_level,
						   unsigned *rx_level);


/**
 * 调整从指定端口接收的信号电平。
 *
 * 应用程序可以调整电平，使从端口接收的信号更响亮或更安静。电平调整用以下公式计算：
 * 		输出=输入*（调整电平+128）/128
 * 		使用这个，零表示没有调整，值-128将使信号静音，+128将使信号变大100%，+256将使信号变大200%，等等
 *
 * 水平调整值将保持在端口上，直到端口从桥上拆下或设置新调整值。调用 pjmedia_conf_get_port_info() 函数时，媒体端口信息中会报告当前的级别调整值
 *
 * @param conf		会议桥
 * @param slot		端口的插槽号
 * @param adj_level	调整级别，必须大于或等于-128。值为零表示不需要进行电平调整，值-128将使信号静音，+128将使信号变大100%，+256将使信号变大200%，
 * 					等等。有关公式，请参见函数说明
 *
 * @return		PJ_SUCCESS on success.
 */
PJ_DECL(pj_status_t) pjmedia_conf_adjust_rx_level( pjmedia_conf *conf,
						   unsigned slot,
						   int adj_level );


/**
 * 调整要传输到指定端口的信号电平。
 * 应用程序可以调整电平，使传输到端口的信号更响亮或更安静。电平调整用以下公式计算：
 * 	输出=输入*（调整电平+128）/128
 * 使用这个，零表示没有调整，值-128将使信号静音，+128将使信号变大100%，+256将使信号变大200%，等等。
 *
 * 水平调整值将保留在端口上，直到端口从桥上拆下或设置了新的调整值。调用 pjmedia_conf_get_port_info()函数时，媒体端口信息中会报告当前的级别调整值。
 *
 * @param conf		会议桥
 * @param slot		端口的插槽号
 * @param adj_level	调整级别，必须大于或等于-128。值为零表示不需要进行电平调整，值-128将使信号静音，+128将使信号变大100%，+256将使信号变大200%，
 * 					等等。有关公式，请参见函数说明
 *
 * @return		PJ_SUCCESS on success.
 */
PJ_DECL(pj_status_t) pjmedia_conf_adjust_tx_level( pjmedia_conf *conf,
						   unsigned slot,
						   int adj_level );


/**
 * 调整从源插槽传输到接收器插槽的信号电平。
 * 应用程序可以调整电平，使从源插槽传输到接收器插槽的信号更响亮或更安静。电平调整用以下公式计算：
 * 	输出=输入*（调整电平+128）/128
 * 使用这个，零表示没有调整，值-128将使信号静音，+128将使信号变大100%，+256将使信号变大200%，等等。
 *
 * 水平调整值将保留在接头上，直到拆下接头或设置新的调整值。调用 pjmedia_conf_get_port_info() 函数时，媒体端口信息中会报告当前的级别调整值。
 *
 * @param conf		The conference bridge.
 * @param src_slot	源
 * @param sink_slot	汇
 * @param adj_level	调整级别，必须大于或等于-128。值为零表示不需要进行电平调整，值-128将使信号静音，+128将使信号变大100%，+256将使信号变大200%，
 * 					等等。有关公式，请参见函数说明
 *
 * @return		PJ_SUCCESS on success.
 */
PJ_DECL(pj_status_t) pjmedia_conf_adjust_conn_level( pjmedia_conf *conf,
						     unsigned src_slot,
						     unsigned sink_slot,
						     int adj_level );


PJ_DECL(pj_status_t) pjmedia_conf_set_pause_sound_cb(pjmedia_conf *conf,
void(*pause_sound)());

PJ_DECL(pj_status_t) pjmedia_conf_set_resume_sound_cb(pjmedia_conf *conf,
void(*resume_sound)());



PJ_END_DECL


/**
 * @}
 */


#endif	/* __PJMEDIA_CONF_H__ */

