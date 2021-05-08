/* $Id: socket.h 5692 2017-11-13 06:06:25Z ming $ */
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
#ifndef __PJ_COMPAT_SOCKET_H__
#define __PJ_COMPAT_SOCKET_H__

/**
 * @file socket.h
 * @brief 提供所有套接字相关功能、数据类型、错误代码等
 */

#if defined(PJ_HAS_WINSOCK2_H) && PJ_HAS_WINSOCK2_H != 0
#  include <winsock2.h>
#endif

#if defined(PJ_HAS_WINSOCK_H) && PJ_HAS_WINSOCK_H != 0
#  include <winsock.h>
#endif

#if defined(PJ_HAS_WS2TCPIP_H) && PJ_HAS_WS2TCPIP_H != 0
#   include <ws2tcpip.h>
#endif


/*
 * 用于 Visual Studio的IPv6
 *
 * = Visual Studio 6 =
 *
 * Visual Studio 6不附带IPv6支持，因此您必须从以下位置下载并安装IPv6技术预览（IPv6Kit）：
 *    http://msdn.microsoft.com/downloads/sdks/platform/tpipv6/ReadMe.asp
 * 然后将 IPv6Kit\inc 放入Visual Studio包含路径
 * 
 * 另外，默认情况下IPv6Kit不希望安装在 Windows 2000 SP4。请看：
 *    http://msdn.microsoft.com/downloads/sdks/platform/tpipv6/faq.asp
 * 介绍如何在Win2K SP4上安装IPv6Kit
 *
 *
 * = Visual Studio 2003, 2005 (including Express) =
 *
 * 这些VS使用用于Windows Server 2003 SP1 的MicrosoftPlatform SDK，并且具有内置的IPv6支持
 */
#if defined(_MSC_VER) && defined(PJ_HAS_IPV6) && PJ_HAS_IPV6!=0
#   ifndef s_addr
#	define s_addr  S_un.S_addr
#   endif

#   if !defined(IPPROTO_IPV6) && (_WIN32_WINNT == 0x0500)
	/* Need to download and install IPv6Kit for this platform.
	 * Please see the comments above about Visual Studio 6.
	 */
#	include <tpipv6.h>
#   endif

#   define PJ_SOCK_HAS_GETADDRINFO  1
#endif	/* _MSC_VER */

#if defined(PJ_HAS_SYS_TYPES_H) && PJ_HAS_SYS_TYPES_H != 0
#  include <sys/types.h>
#endif

#if defined(PJ_HAS_SYS_SOCKET_H) && PJ_HAS_SYS_SOCKET_H != 0
#  include <sys/socket.h>
#endif

#if defined(PJ_HAS_LINUX_SOCKET_H) && PJ_HAS_LINUX_SOCKET_H != 0
#  include <linux/socket.h>
#endif

#if defined(PJ_HAS_SYS_SELECT_H) && PJ_HAS_SYS_SELECT_H != 0
#  include <sys/select.h>
#endif

#if defined(PJ_HAS_NETINET_IN_H) && PJ_HAS_NETINET_IN_H != 0
#  include <netinet/in.h>
#endif

#if defined(PJ_HAS_NETINET_IN_SYSTM_H) && PJ_HAS_NETINET_IN_SYSTM_H != 0
/* 需要在FreeBSD 7.0中包含NetNet/ip.h */
#  include <netinet/in_systm.h>
#endif

#if defined(PJ_HAS_NETINET_IP_H) && PJ_HAS_NETINET_IP_H != 0
/* To pull in IPTOS_* constants */
#  include <netinet/ip.h>
#endif

#if defined(PJ_HAS_NETINET_TCP_H) && PJ_HAS_NETINET_TCP_H != 0
/* To pull in TCP_NODELAY constants */
#  include <netinet/tcp.h>
#endif

#if defined(PJ_HAS_NET_IF_H) && PJ_HAS_NET_IF_H != 0
/* For interface enumeration in ip_helper */
#   include <net/if.h>
#endif

#if defined(PJ_HAS_IFADDRS_H) && PJ_HAS_IFADDRS_H != 0
/* Interface enum with getifaddrs() which works with IPv6 */
#   include <ifaddrs.h>
#endif

#if defined(PJ_HAS_ARPA_INET_H) && PJ_HAS_ARPA_INET_H != 0
#  include <arpa/inet.h>
#endif

#if defined(PJ_HAS_SYS_IOCTL_H) && PJ_HAS_SYS_IOCTL_H != 0
#  include <sys/ioctl.h>	/* FBIONBIO */
#endif

#if defined(PJ_HAS_ERRNO_H) && PJ_HAS_ERRNO_H != 0
#  include <errno.h>
#endif

#if defined(PJ_HAS_NETDB_H) && PJ_HAS_NETDB_H != 0
#  include <netdb.h>
#endif

#if defined(PJ_HAS_UNISTD_H) && PJ_HAS_UNISTD_H != 0
#  include <unistd.h>
#endif

#if defined(PJ_HAS_SYS_FILIO_H) && PJ_HAS_SYS_FILIO_H != 0
#   include <sys/filio.h>
#endif

#if defined(PJ_HAS_SYS_SOCKIO_H) && PJ_HAS_SYS_SOCKIO_H != 0
#   include <sys/sockio.h>
#endif


/*
 * 定义常见错误
 */
#if (defined(PJ_WIN32) && PJ_WIN32!=0) || \
    (defined(PJ_WIN32_WINCE) && PJ_WIN32_WINCE!=0) || \
    (defined(PJ_WIN64) && PJ_WIN64!=0)
#  define OSERR_EWOULDBLOCK    WSAEWOULDBLOCK
#  define OSERR_EINPROGRESS    WSAEINPROGRESS
#  define OSERR_ECONNRESET     WSAECONNRESET
#  define OSERR_ENOTCONN       WSAENOTCONN
#  define OSERR_EAFNOSUPPORT   WSAEAFNOSUPPORT
#  define OSERR_ENOPROTOOPT    WSAENOPROTOOPT
#elif defined(PJ_SYMBIAN) && PJ_SYMBIAN!=0
#  define OSERR_EWOULDBLOCK    -1
#  define OSERR_EINPROGRESS    -1
#  define OSERR_ECONNRESET     -1
#  define OSERR_ENOTCONN       -1
#  define OSERR_EAFNOSUPPORT   -1
#  define OSERR_ENOPROTOOPT    -1
#else
#  define OSERR_EWOULDBLOCK    EWOULDBLOCK
#  define OSERR_EINPROGRESS    EINPROGRESS
#  define OSERR_ECONNRESET     ECONNRESET
#  define OSERR_ENOTCONN       ENOTCONN
#  define OSERR_EAFNOSUPPORT   EAFNOSUPPORT
#  define OSERR_ENOPROTOOPT    ENOPROTOOPT
#endif


/*
 * 取消这些定义
 */
#undef s_addr
#undef s6_addr
#undef sin_zero

/*
 * 这将最终被淘汰，因为它应该在 os_auto.h 中声明
 */
#if !defined(PJ_HAS_SOCKLEN_T) || PJ_HAS_SOCKLEN_T==0
    typedef int socklen_t;
#endif

/*
 * 关于sockaddr_in 的 sin_len成员：
 *  BSD系统（包括MacOS X）要求将 sockaddr_in的 sin_len 成员设置为sizeof(sockaddr_in)，而其他系统（包括Windows和Linux）则不要求
 *  为了保持系统之间的兼容性，PJLIB 将在调用本机OS socket API之前自动设置此字段，并且在将 pj_sockaddr_in 返回到应用程序之前（例如在pj_getsockname()
 *  和pj_recvfrom() 中）始终将此字段重置为零。
 *
 *  应用程序必须始终将此字段设置为零
 *  这样就可以避免使用套接字地址作为哈希表密钥时难以发现的问题
 */
#if defined(PJ_SOCKADDR_HAS_LEN) && PJ_SOCKADDR_HAS_LEN!=0
#   define PJ_SOCKADDR_SET_LEN(addr,len) (((pj_addr_hdr*)(addr))->sa_zero_len=(len))
#   define PJ_SOCKADDR_RESET_LEN(addr)   (((pj_addr_hdr*)(addr))->sa_zero_len=0)
#else
#   define PJ_SOCKADDR_SET_LEN(addr,len) 
#   define PJ_SOCKADDR_RESET_LEN(addr)
#endif

#endif	/* __PJ_COMPAT_SOCKET_H__ */

