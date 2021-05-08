/* $Id: os_linux.h 5458 2016-10-13 04:32:29Z riza $ */
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
#ifndef __PJ_COMPAT_OS_LINUX_H__
#define __PJ_COMPAT_OS_LINUX_H__

/**
 * @file os_linux.h
 * @brief 描述Linux操作系统的细节
 */

#define PJ_OS_NAME		    "linux"

#define PJ_HAS_ARPA_INET_H	    1
#define PJ_HAS_ASSERT_H		    1
#define PJ_HAS_CTYPE_H		    1
#define PJ_HAS_ERRNO_H		    1
#define PJ_HAS_LINUX_SOCKET_H	    0
#define PJ_HAS_MALLOC_H		    1
#define PJ_HAS_NETDB_H		    1
#define PJ_HAS_NETINET_IN_H	    1
#define PJ_HAS_SETJMP_H		    1
#define PJ_HAS_STDARG_H		    1
#define PJ_HAS_STDDEF_H		    1
#define PJ_HAS_STDIO_H		    1
#define PJ_HAS_STDLIB_H		    1
#define PJ_HAS_STRING_H		    1
#define PJ_HAS_SYS_IOCTL_H	    1
#define PJ_HAS_SYS_SELECT_H	    1
#define PJ_HAS_SYS_SOCKET_H	    1
#define PJ_HAS_SYS_TIME_H	    0
#define PJ_HAS_SYS_TIMEB_H	    1
#define PJ_HAS_SYS_TYPES_H	    1
#define PJ_HAS_TIME_H		    1
#define PJ_HAS_UNISTD_H		    1
#define PJ_HAS_SEMAPHORE_H	    1

#define PJ_HAS_MSWSOCK_H	    0
#define PJ_HAS_WINSOCK_H	    0
#define PJ_HAS_WINSOCK2_H	    0

#define PJ_HAS_LOCALTIME_R	    1

#define PJ_SOCK_HAS_INET_ATON	    1

/* 如果中的本机sockaddr_in 具有sin_len成员，则设置1
 * 默认: 0
 */
#define PJ_SOCKADDR_HAS_LEN	    0

/**
 * 如果设置了这个宏，它会告诉 select I/O Queue select（）需要给定正确的 nfds 值（即最大fd+1）。
 * 这需要select ioqueue 来重新扫描每个注册和注销的描述符。
 *
 * 如果未设置此宏，则在调用select()时，ioqueue将始终为 nfds 参数提供FD_SETSIZE
 *
 * 默认: 0
 */
#define PJ_SELECT_NEEDS_NFDS	    0

/*
 * errno 是检索操作系统错误的好方法吗？
 */
#define PJ_HAS_ERRNO_VAR	    1

/*
 * 设置此宏时，getsockopt(SOL_SOCKET, SO_ERROR) 将返回非阻塞connect（）操作的状态
 */
#define PJ_HAS_SO_ERROR             1

/*
 * 此值指定当非阻塞套接字 recv() 无法返回立即数据时，操作系统在 errno 中设置的值
 */
#define PJ_BLOCKING_ERROR_VAL       EAGAIN

/*
 * 当无法立即连接非阻塞套接字connect（）时，此值指定操作系统在 errno 中设置的值
 */
#define PJ_BLOCKING_CONNECT_ERROR_VAL   EINPROGRESS

/* 默认线程是启用的，除非它被重写 */
#ifndef PJ_HAS_THREADS
#  define PJ_HAS_THREADS	    (1)
#endif

#define PJ_HAS_HIGH_RES_TIMER	    1
#define PJ_HAS_MALLOC               1
#ifndef PJ_OS_HAS_CHECK_STACK
#   define PJ_OS_HAS_CHECK_STACK    0
#endif
#define PJ_NATIVE_STRING_IS_UNICODE 0

#define PJ_ATOMIC_VALUE_TYPE	    long

/* 如果为1，则对不支持读/写互斥模拟的平台使用读/写互斥模拟 */
#define PJ_EMULATE_RWMUTEX	    0

/*
 * 如果为1，pj_thread_create() 应在创建线程时强制堆栈大小
 * Default: 0 (让操作系统决定线程的堆栈大小).
 */
#define PJ_THREAD_SET_STACK_SIZE    	0

/*
 * 如果为1，pj_thread_create() 应该从提供的池中分配堆栈。
 * 默认值：0（让操作系统为线程堆栈分配内存）
 */
#define PJ_THREAD_ALLOCATE_STACK    	0

/* Linux 有 socklen_t */
#define PJ_HAS_SOCKLEN_T		1


#endif	/* __PJ_COMPAT_OS_LINUX_H__ */

