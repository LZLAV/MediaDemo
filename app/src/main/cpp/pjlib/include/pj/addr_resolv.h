/**
 * 已完成
 * 	地址解析
 */
#ifndef __PJ_ADDR_RESOLV_H__
#define __PJ_ADDR_RESOLV_H__

/**
 * @file addr_resolv.h
 * @brief IP地址解析
 */

#include <pj/sock.h>

PJ_BEGIN_DECL

/**
 * @defgroup pj_addr_resolve 网络地址解析
 * @ingroup PJ_IO
 * @{
 *
 * 此模块提供解析指定主机名的 Internet地址的功能。要解析特定的主机名，应用程序只需调用pj_gethostbyname()
 *
 * Example:
 * <pre>
 *   ...
 *   pj_hostent he;
 *   pj_status_t rc;
 *   pj_str_t host = pj_str("host.example.com");
 *   
 *   rc = pj_gethostbyname( &host, &he);
 *   if (rc != PJ_SUCCESS) {
 *      char errbuf[80];
 *      pj_strerror( rc, errbuf, sizeof(errbuf));
 *      PJ_LOG(2,("sample", "Unable to resolve host, error=%s", errbuf));
 *      return rc;
 *   }
 *
 *   // process address...
 *   addr.sin_addr.s_addr = *(pj_uint32_t*)he.h_addr;
 *   ...
 * </pre>
 *
 * 真的很简单。。。
 */

/** 此结构描述Internet主机地址 */
typedef struct pj_hostent
{
    char    *h_name;		/**< 主机的正式名字 */
    char   **h_aliases;		/**< 别名列表 */
    int	     h_addrtype;	/**< 主机地址的类型 */
    int	     h_length;		/**< 地址的长度 */
    char   **h_addr_list;	/**< 地址列表 */
} pj_hostent;

/** h_addr_list[0] 的别名 */
#define h_addr h_addr_list[0]

/** 
 * 这个结构描述地址信息 pj_getaddrinfo()
 */
typedef struct pj_addrinfo
{
    char	 ai_canonname[PJ_MAX_HOSTNAME]; /**< 主机的规范名称 */
    pj_sockaddr  ai_addr;			/**< 二进制地址	    */
} pj_addrinfo;


/**
 * 此函数用于填充给定主机名的 pj_hostent 类型的结构。
 * 有关同样适用于IPv6的主机解析函数，请参阅 pj_getaddrinfo()
 *
 * @param name	    要解析的主机名。在某些平台（如Windows）上，在此处指定IPv4地址可能会失败
 * @param he	    要填充的 pj_hostent 结构。请注意，此结构中的指针指向临时变量，该值将在后续调用时重置
 *
 * @return	    成功返回 PJ_SUCCESS，否则返回相应错误码
 */ 
PJ_DECL(pj_status_t) pj_gethostbyname(const pj_str_t *name, pj_hostent *he);


/**
 * 解析本地主机的主IP地址
 *
 * @param af	    地址族。有效值为 pj_AF_INET() 或 pj_AF_INET6()
 * @param addr      成功解析后，此套接字地址的地址族和地址部分将按网络字节顺序填充主机IP地址。套接字地址的其他部分是不变的
 *
 * @return	    成功返回PJ_SUCCESS，否则返回相应错误码
 */
PJ_DECL(pj_status_t) pj_gethostip(int af, pj_sockaddr *addr);


/**
 * 获取接口IP地址以将数据发送到指定的目标
 *
 * @param af	    所需地址协议族的查询，有效值为 pj_AF_INET() 或 pj_AF_INET6().
 * @param dst	    目的主机
 * @param itf_addr  成功解析后，此套接字地址的地址族和地址部分将按网络字节顺序填充主机IP地址。应该忽略套接字地址的其他部分
 * @param allow_resolve   如果 dst 可能包含主机名（而不是IP地址），请指定是否应执行主机名解析。否则，将返回默认接口地址
 * @param p_dst_addr 如果不为空，则用目标主机的IP地址填充
 *
 * @return	    成功返回 PJ_SUCCESS，否则返回相应的错误码
 */
PJ_DECL(pj_status_t) pj_getipinterface(int af,
                                       const pj_str_t *dst,
                                       pj_sockaddr *itf_addr,
                                       pj_bool_t allow_resolve,
                                       pj_sockaddr *p_dst_addr);

/**
 * 获取默认接口的IP地址。默认接口是默认路由的接口
 *
 * @param af	    所需地址协议族的查询，有效值为 pj_AF_INET() 或 pj_AF_INET6()
 * @param addr      成功解析后，此套接字地址的地址族和地址部分将按网络字节顺序填充主机IP地址。套接字地址的其他部分是不变的
 *
 * @return	    成功返回 PJ_SUCCESS，否则返回相应错误码
 */
PJ_DECL(pj_status_t) pj_getdefaultipinterface(int af,
					      pj_sockaddr *addr);


/**
 * 此函数用于转换服务位置的名称（例如，主机名），并返回一组地址和相关信息，用于创建用于为指定服务寻址的套接字
 *
 * @param af	    所需地址协议族的查询，有效值为 pj_AF_INET(), pj_AF_INET6() 或 pj_AF_UNSPEC()
 * @param name	    描述性名称或地址字符串，如主机名
 * @param count	    输入时，它指定ai数组中的元素数。在输出时，将使用为指定名称找到的地址信息数进行设置
 * @param ai	    要用主机信息填充的地址信息数组
 *
 * @return	    成功返回 PJ_SUCCESS，否则返回相应错误码
 */
PJ_DECL(pj_status_t) pj_getaddrinfo(int af, const pj_str_t *name,
				    unsigned *count, pj_addrinfo ai[]);



/** @} */

PJ_END_DECL

#endif	/* __PJ_ADDR_RESOLV_H__ */

