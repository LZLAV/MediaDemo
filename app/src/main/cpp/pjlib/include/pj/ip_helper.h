/**
 * 已完成
 * 		IP 帮助
 */
#ifndef __PJ_IP_ROUTE_H__
#define __PJ_IP_ROUTE_H__

/**
 * @file ip_helper.h
 * @brief IP helper API
 */

#include <pj/sock.h>

PJ_BEGIN_DECL

/**
 * @defgroup pj_ip_helper IP 接口和路由的帮助类
 * @ingroup PJ_IO
 * @{
 *
 * 本模块提供查询本地主机IP接口和路由表的功能。
 */

/**
 * 此结构描述IP路由条目
 */
typedef union pj_ip_route_entry
{
    /** IPv4 路由 */
    struct
    {
	pj_in_addr	if_addr;    /**< 本地接口IP 地址	*/
	pj_in_addr	dst_addr;   /**< 目的IP 地址	*/
	pj_in_addr	mask;	    /**< 目的掩码		*/
    } ipv4;
} pj_ip_route_entry;


/**
 * 枚举主机中当前活动的本地IP接口
 *
 * @param af	要检索的地址的族。应用程序可以指定pj_AF_UNSPEC()来检索所有地址，或者指定pj_AF_INET()或pj_AF_INET6()来检索具有特定地址族的接口
 * @param count	    输入时，指定条目数。在输出时，它将填充实际的条目数
 * @param ifs	    套接字地址数组，地址部分将填充接口地址。地址族部分将用 IP地址的地址族初始化
 *
 * @return	    成功返回 PJ_SUCCES，否则返回相应错误码
 */
PJ_DECL(pj_status_t) pj_enum_ip_interface(int af,
					  unsigned *count,
					  pj_sockaddr ifs[]);


/**
 * 枚举此主机的IP路由表
 *
 * @param count	    输入时，指定路由条目数。在输出时，它将填充路由条目的实际数量
 * @param routes    IP路由条目数组
 *
 * @return	    成功返回 PJ_SUCCES，否则返回相关错误码
 */
PJ_DECL(pj_status_t) pj_enum_ip_route(unsigned *count,
				      pj_ip_route_entry routes[]);



/** @} */

PJ_END_DECL


#endif	/* __PJ_IP_ROUTE_H__ */

