/**
 * 已看完
 */
#ifndef __PJ_SOCK_QOS_H__
#define __PJ_SOCK_QOS_H__

/**
 * @file sock_qos.h
 * @brief Socket QoS API
 */

#include <pj/sock.h>

PJ_BEGIN_DECL 


/**
 * @defgroup socket_qos Socket 服务质量(QoS) API: TOS, DSCP, WMM, IEEE 802.1p
 * @ingroup PJ_SOCK
 * @{


    \section Qos技术简介

    QoS设置可用于TCP/IP协议的第2层和第3层：

    \subsection 简介IEEE802.1p第2层：用于以太网的 IEEE802.1p

    ieee802.1p标记将使用以太网帧的虚拟局域网（VLAN）报头中的3位优先级字段标记主机发送的帧以进行优先级传递。
    VLAN 报头位于以太网报头内部，位于源地址字段和长度字段（对于IEEE 802.3帧）或以太网类型字段（对于Ethernet II帧）之间。

    \subsection 简介WMM层 2: WMM （Wi-Fi 多媒体联盟）

    在ieee802.11无线网络接口层，Wi-Fi多媒体联盟认证（Wi-Fi Alliance certification for Wi-Fi Multimedia，WMM）
    定义了四种用于优先处理网络流量的访问类别。这些访问类别是（按优先级从高到低的顺序）语音、视频、尽力而为和背景。主机对WMM
    优先级的支持要求无线网络适配器及其驱动程序都支持WMM。无线接入点（AP）必须启用WMM。

    \subsection 简介dscp 3 层: DSCP

    在 Internet层，您可以使用区分Qos并在IP报头中设置differential Services代码点（DSCP）的值。
    如RFC 2474中所定义，DSCP值是IPv4 TOS字段和IPv6 Traffic Class字段的高位6位。

    \subsection 简介其他 3层: 其他

    存在其他机制（如RSVP、IntServ），但不会实现


    \section Qos 可用性

    \subsection Linux

    DSCP通过IP TOS选项提供

 	以太网 802.1p 标记是通过设置套接字的 setsockopt（SO_PRIORITY）选项来完成的，
 	然后使用vconfig 程序的set_egress_map 选项将其转换为数据包中设置 vlan_qos字段。

    尚不清楚 WMM 是否可用

    \subsection Windows and Windows Mobile

    (It's a mess!)

    在Windows 2000或更旧版本上，可以使用setsockopt（）设置DSCP，但在WinXP或更高版本上，
    Windows将自动忽略此调用，除非管理员修改注册表。在Windows 2000、Windows XP和Windows Server 2003上，
    GQoS（通用QoS）API是标准API，但将来可能不支持此API。在Vista和windows7上，是一个新的qos2api，
    也称为优质Windows音频视频体验（qWAVE）。

 	IEEE 802.1p 标记可通过在Windows XP SP2上提供的流量控制（TC）API获得，但这需要管理员访问。
 	对于 Vista 和更高版本，它在qWAVE中。

 	WMM通过 setsockopt(IP_DSCP_TRAFFIC_TYPE) 可用于Windows mobile 6平台和Windows Embedded CE 6上的移动平台。
 	qWAVE也支持这一点。

    \subsection symbian Symbian S60 3rd Ed


	DSCP和WMM都通过 RSocket::SetOpt() 支持，它将相应地设置第 2 层和第 3 层的QoS设置。
 	在内部，PJLIB设置套接字的 DSCP字段，并且基于某些 DSCP值映射，Symbian将相应地设置 WMM标记。

    \section PJLIB的 QoS api抽象

    在此基础上，实现了以下API。声明以下“标准”流量类型。
    \code
     typedef enum pj_qos_type
     {
	PJ_QOS_TYPE_BEST_EFFORT,		尽力而为
	PJ_QOS_TYPE_BACKGROUND,			背景
	PJ_QOS_TYPE_VIDEO,				视频
	PJ_QOS_TYPE_VOICE,				语音
	PJ_QOS_TYPE_CONTROL,			控制
	PJ_QOS_TYPE_SIGNALLING			信号
     } pj_qos_type;
    \endcode

    上面的流量类别将决定如何使用第2层和第3层的QoS设置。上述类别与相应的第2层和第3层设置之间的标准映射如下：

    \code
    =================================================================
    PJLIB流量类型 		IP DSCP 		WMM 		   802.1p
    -----------------------------------------------------------------
    BEST_EFFORT 		0x00 		BE (Bulk Effort) 	0
    BACKGROUND 			0x08 		BK (Bulk) 			2
    VIDEO 				0x28 		VI (Video) 			5
    VOICE 				0x30 		VO (Voice) 			6
    CONTROL 			0x38 		VO (Voice) 			7
    SIGNALLING 			0x28 		VI (Video) 			5
    =================================================================
    \endcode

 	提供了两组API来操作QoS参数。

    \subsection 可移植API

    第一套API:

    \code
     // 设置 QoS 参数
     PJ_DECL(pj_status_t) pj_sock_set_qos_type(pj_sock_t sock,
					       pj_qos_type val);

     // 获取 QoS 参数
     PJ_DECL(pj_status_t) pj_sock_get_qos_type(pj_sock_t sock,
					       pj_qos_type *p_val);
    \endcode

 	API将根据 DSCP类为第2层和第3层 QoS设置（如果可用）设置流量类型。如果任何一个层 QoS设置不可设置，
 	API将自动忽略它。如果两个层都不可设置，API将返回错误。

 	上面的API是推荐使用的QoS，因为它在所有平台上都是最可移植的

    \subsection 详细API

 	第二组API用于希望微调QoS参数的应用程序

 	第 2 层和第 3 层QoS参数存储在 pj_qos_params 结构中：

    \code
     typedef enum pj_qos_flag
     {
	PJ_QOS_PARAM_HAS_DSCP = 1,
	PJ_QOS_PARAM_HAS_SO_PRIO = 2,
	PJ_QOS_PARAM_HAS_WMM = 4
     } pj_qos_flag;

     typedef enum pj_qos_wmm_prio
     {
	PJ_QOS_WMM_PRIO_BULK_EFFORT,
	PJ_QOS_WMM_PRIO_BULK,
	PJ_QOS_WMM_PRIO_VIDEO,
	PJ_QOS_WMM_PRIO_VOICE
     } pj_qos_wmm_prio;

     typedef struct pj_qos_params
     {
	pj_uint8_t      flags;    // 确定要设置的值，pj_qos_flag 位
	pj_uint8_t      dscp_val; // 设置 6位 DSCP值
	pj_uint8_t      so_prio;  // SO_PRIORITY 值
	pj_qos_wmm_prio wmm_prio; // WMM priority 值
     } pj_qos_params;
    \endcode

    第二组API对参数进行更详细的设置，包括：

    \code
     // 检索指定流量类型的QoS参数
     PJ_DECL(pj_status_t) pj_qos_get_params(pj_qos_type type, 
					    pj_qos_params *p);

     // 设置Socket 的 Qos 参数
     PJ_DECL(pj_status_t) pj_sock_set_qos_params(pj_sock_t sock,
						 const pj_qos_params *p);

     // 从Socket 获取 Qos 参数
     PJ_DECL(pj_status_t) pj_sock_get_qos_params(pj_sock_t sock,
						 pj_qos_params *p);
    \endcode


    重要:
		pj_sock_set/get_qos_params() API是不可移植的，它可能只能在Linux上实现。
		应用程序应始终尝试 pj_sock_set_qos_type() 。
 */


/**
 * 流量分类
 */
typedef enum pj_qos_type
{
    PJ_QOS_TYPE_BEST_EFFORT,	/**< 尽力而为 */
    PJ_QOS_TYPE_BACKGROUND,	/**< 背景	*/
    PJ_QOS_TYPE_VIDEO,		/**< 视频		*/
    PJ_QOS_TYPE_VOICE,		/**< 语音		*/
    PJ_QOS_TYPE_CONTROL,	/**< 控制		*/
    PJ_QOS_TYPE_SIGNALLING	/**< 信号	*/
} pj_qos_type;

/**
 * pj_qos_params 中字段 flags 的Qos参数设置值
 */
typedef enum pj_qos_flag
{
    PJ_QOS_PARAM_HAS_DSCP = 1,	    /**< 设置支持 DSCP 参数    */
    PJ_QOS_PARAM_HAS_SO_PRIO = 2,   /**< 设置支持优先级 Socket SO_PRIORITY 	    */
    PJ_QOS_PARAM_HAS_WMM = 4	    /**< 设置支持Wi-Fi 	    */
} pj_qos_flag;


/**
 * 标准的 Wi-Fi 优先级
 */
typedef enum pj_qos_wmm_prio
{
    PJ_QOS_WMM_PRIO_BULK_EFFORT,	/**< Bulk effort priority   */
    PJ_QOS_WMM_PRIO_BULK,		/**< Bulk priority.	    */
    PJ_QOS_WMM_PRIO_VIDEO,		/**< Video priority	    */
    PJ_QOS_WMM_PRIO_VOICE		/**< Voice priority	    */
} pj_qos_wmm_prio;


/**
 * QoS 参数（Socket中）
 */
typedef struct pj_qos_params
{
    pj_uint8_t      flags;    /** 确定支持的类别 */
    pj_uint8_t      dscp_val; /**< DSCP 设置的 6位 */
    pj_uint8_t      so_prio;  /**< SO_PRIORITY 值 */
    pj_qos_wmm_prio wmm_prio; /**< WMM 优先级  */
} pj_qos_params;



/**
 * 设置 socket qos type
 *
 * @param sock	    The socket.
 * @param type	    流量控制类别
 *
 * @return	    PJ_SUCCESS 第2层或第3层设置成功，则返回 PJ_SUCCESS;否则返回 PJ_FALSE
 */
PJ_DECL(pj_status_t) pj_sock_set_qos_type(pj_sock_t sock,
					  pj_qos_type type);

/**
 * 在使用低级API修改第2层 或第3层设置的情况下，此函数可能返回与设置匹配的最近QoS类型的近似值。
 *
 * @param sock	    The socket.
 * @param p_type    Pointer to receive the traffic type of the socket.
 *
 * @return	    PJ_SUCCESS 可从socket 获取流量类型或者近似的类型则返回 PJ_SUCCESS
 */
PJ_DECL(pj_status_t) pj_sock_get_qos_type(pj_sock_t sock,
					  pj_qos_type *p_type);


/**
 * 用于将QoS应用于套接字
 *
 * @param sock		socket句柄
 * @param qos_type	QoS traffic type. 不是 PJ_QOS_TYPE_BEST_EFFORT 则会应用
 * @param qos_params	可选的低级QoS参数。仅当此参数不为 NULL且结构中的标志不为零时，才会应用此选项。返回时，标志将指示哪些参数已成功应用。
 * @param log_level	日志级别
 * @param log_sender 日志tag
 * @param sock_name	标识 socket 的名称
 *
 * @return		PJ_SUCCESS 如果至少第2层或第3层设置成功。如果第2层和第3层设置都无法设置，此函数将返回错误。
 *
 * @see pj_sock_apply_qos2()
 */
PJ_DECL(pj_status_t) pj_sock_apply_qos(pj_sock_t sock,
				       pj_qos_type qos_type,
				       pj_qos_params *qos_params,
				       unsigned log_level,
				       const char *log_sender,
				       const char *sock_name);

/**
 * pj_sock_apply_qos() 的重载,qos_params --> const qos_params
 *
 * @see pj_sock_apply_qos()
 */
PJ_DECL(pj_status_t) pj_sock_apply_qos2(pj_sock_t sock,
 				        pj_qos_type qos_type,
				        const pj_qos_params *qos_params,
				        unsigned log_level,
				        const char *log_sender,
				        const char *sock_name);

/**
 * 检索指定流量类型的 QoS参数的标准映射
 *
 * @param type	    流量类型
 * @param p_param   检索Qos参数的指针
 *
 * @return	    成功返回 PJ_SUCCESS 或者返回相应错误码
 */ 
PJ_DECL(pj_status_t) pj_qos_get_params(pj_qos_type type, 
				       pj_qos_params *p_param);


/**
 * 检索与指定QoS参数匹配的通信量类型。如果找不到精确匹配，则此函数将返回指定QoS参数的最近匹配流量类型的近似值
 *
 * @param param	    包含要映射到“标准”流量类型的QoS参数的结构
 * @param p_type    接收通信量类型的指针
 *
 * @return	    成功返回 PJ_SUCCESS 或者返回相应错误码
 */ 
PJ_DECL(pj_status_t) pj_qos_get_type(const pj_qos_params *param,
				     pj_qos_type *p_type);


/**
 * 这是一个低级别API，用于设置Socket的 Qos参数
 *
 * @param sock	    Socket句柄
 * @param param	    用于Socket的QoS参数。返回时，此结构的标志字段将设置位掩码值，该值指示哪些QoS设置已成功应用于套接字。
 * @return	    PJ_SUCCESS 至少设置成功一个字段，则返回 PJ_SUCCESS;否则返回错误
 */ 
PJ_DECL(pj_status_t) pj_sock_set_qos_params(pj_sock_t sock,
					    pj_qos_params *param);

/**
 * 低级api，从Socket获取QoS参数
 *
 * @param sock	    socket句柄
 * @param p_param   接收参数的指针。成功返回后，将使用适当的位掩码初始化此结构的flags字段，以指示已成功检索到哪些字段。
 *
 * @return	    成功返回 PJ_SUCCESS 或者返回相应错误码
 */
PJ_DECL(pj_status_t) pj_sock_get_qos_params(pj_sock_t sock,
					    pj_qos_params *p_param);


/**
 * @}
 */


PJ_END_DECL

#endif	/* __PJ_SOCK_QOS_H__ */

