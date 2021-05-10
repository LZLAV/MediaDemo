/**
 * 已完成：
 *
 */
#ifndef __PJ_CONFIG_H__
#define __PJ_CONFIG_H__


/**
 * @file config.h
 * @brief PJLIB 主配置设置
 */

/********************************************************************
 * 包括编译器特定的配置
 */
#if defined(_MSC_VER)
#  include <pj/compat/cc_msvc.h>
#elif defined(__GNUC__)
#  include <pj/compat/cc_gcc.h>
#elif defined(__CW32__)
#  include <pj/compat/cc_mwcc.h>
#elif defined(__MWERKS__)
#  include <pj/compat/cc_codew.h>
#elif defined(__GCCE__)
#  include <pj/compat/cc_gcce.h>
#elif defined(__ARMCC__)
#  include <pj/compat/cc_armcc.h>
#else
#  error "Unknown compiler."
#endif

/* PJ_ALIGN_DATA 是编译器特定的用于对齐数据地址的指令 */
#ifndef PJ_ALIGN_DATA
#  error "PJ_ALIGN_DATA is not defined!"
#endif

/********************************************************************
 * 包括特定于目标操作系统的配置
 */
#if defined(PJ_AUTOCONF)
    /*
     * Autoconf
     */
#   include <pj/compat/os_auto.h>

#elif defined(PJ_SYMBIAN) && PJ_SYMBIAN!=0
    /*
     * SymbianOS
     */
#  include <pj/compat/os_symbian.h>

#elif defined(PJ_WIN32_WINCE) || defined(_WIN32_WCE) || defined(UNDER_CE)
    /*
     * Windows CE
     */
#   undef PJ_WIN32_WINCE
#   define PJ_WIN32_WINCE   1
#   include <pj/compat/os_win32_wince.h>

    /* Also define Win32 */
#   define PJ_WIN32 1

#elif defined(PJ_WIN32_WINPHONE8) || defined(_WIN32_WINPHONE8)
    /*
     * Windows Phone 8
     */
#   undef PJ_WIN32_WINPHONE8
#   define PJ_WIN32_WINPHONE8   1
#   include <pj/compat/os_winphone8.h>

    /* Also define Win32 */
#   define PJ_WIN32 1

#elif defined(PJ_WIN32_UWP) || defined(_WIN32_UWP)
    /*
     * Windows UWP
     */
#   undef PJ_WIN32_UWP
#   define PJ_WIN32_UWP   1
#   include <pj/compat/os_winuwp.h>

    /* Define Windows phone */
#   define PJ_WIN32_WINPHONE8 1

    /* Also define Win32 */
#   define PJ_WIN32 1

#elif defined(PJ_WIN32) || defined(_WIN32) || defined(__WIN32__) || \
	defined(WIN32) || defined(PJ_WIN64) || defined(_WIN64) || \
	defined(WIN64) || defined(__TOS_WIN__) 
#   if defined(PJ_WIN64) || defined(_WIN64) || defined(WIN64)
	/*
	 * Win64
	 */
#	undef PJ_WIN64
#	define PJ_WIN64 1
#   endif
#   undef PJ_WIN32
#   define PJ_WIN32 1
#   include <pj/compat/os_win32.h>

#elif defined(PJ_LINUX) || defined(linux) || defined(__linux)
    /*
     * Linux
     */
#   undef PJ_LINUX
#   define PJ_LINUX	    1
#   include <pj/compat/os_linux.h>

#elif defined(PJ_PALMOS) && PJ_PALMOS!=0
    /*
     * Palm
     */
#  include <pj/compat/os_palmos.h>

#elif defined(PJ_SUNOS) || defined(sun) || defined(__sun)
    /*
     * SunOS
     */
#   undef PJ_SUNOS
#   define PJ_SUNOS	    1
#   include <pj/compat/os_sunos.h>

#elif defined(PJ_DARWINOS) || defined(__MACOSX__) || \
      defined (__APPLE__) || defined (__MACH__)
    /*
     * MacOS X
     */
#   undef PJ_DARWINOS
#   define PJ_DARWINOS	    1
#   include <pj/compat/os_darwinos.h>

#elif defined(PJ_RTEMS) && PJ_RTEMS!=0
    /*
     * RTEMS
     */
#  include <pj/compat/os_rtems.h>
#else
#   error "Please specify target os."
#endif


/********************************************************************
 * 目标计算机特定配置
 */
#if defined(PJ_AUTOCONF)
    /*
     * Autoconf configured
     */
#include <pj/compat/m_auto.h>

#elif defined (PJ_M_I386) || defined(_i386_) || defined(i_386_) || \
	defined(_X86_) || defined(x86) || defined(__i386__) || \
	defined(__i386) || defined(_M_IX86) || defined(__I86__)
    /*
     * Generic i386 processor family, little-endian
     */
#   undef PJ_M_I386
#   define PJ_M_I386		1
#   define PJ_M_NAME		"i386"
#   define PJ_HAS_PENTIUM	1
#   define PJ_IS_LITTLE_ENDIAN	1
#   define PJ_IS_BIG_ENDIAN	0


#elif defined (PJ_M_X86_64) || defined(__amd64__) || defined(__amd64) || \
	defined(__x86_64__) || defined(__x86_64) || \
	defined(_M_X64) || defined(_M_AMD64)
    /*
     * AMD 64bit processor, little endian
     */
#   undef PJ_M_X86_64
#   define PJ_M_X86_64		1
#   define PJ_M_NAME		"x86_64"
#   define PJ_HAS_PENTIUM	1
#   define PJ_IS_LITTLE_ENDIAN	1
#   define PJ_IS_BIG_ENDIAN	0

#elif defined(PJ_M_IA64) || defined(__ia64__) || defined(_IA64) || \
	defined(__IA64__) || defined( 	_M_IA64)
    /*
     * Intel IA64 processor, default to little endian
     */
#   undef PJ_M_IA64
#   define PJ_M_IA64		1
#   define PJ_M_NAME		"ia64"
#   define PJ_HAS_PENTIUM	1
#   define PJ_IS_LITTLE_ENDIAN	1
#   define PJ_IS_BIG_ENDIAN	0

#elif defined (PJ_M_M68K) && PJ_M_M68K != 0

    /*
     * Motorola m68k processor, big endian
     */
#   undef PJ_M_M68K
#   define PJ_M_M68K		1
#   define PJ_M_NAME		"m68k"
#   define PJ_HAS_PENTIUM	0
#   define PJ_IS_LITTLE_ENDIAN	0
#   define PJ_IS_BIG_ENDIAN	1


#elif defined (PJ_M_ALPHA) || defined (__alpha__) || defined (__alpha) || \
	defined (_M_ALPHA)
    /*
     * DEC Alpha processor, little endian
     */
#   undef PJ_M_ALPHA
#   define PJ_M_ALPHA		1
#   define PJ_M_NAME		"alpha"
#   define PJ_HAS_PENTIUM	0
#   define PJ_IS_LITTLE_ENDIAN	1
#   define PJ_IS_BIG_ENDIAN	0


#elif defined(PJ_M_MIPS) || defined(__mips__) || defined(__mips) || \
	defined(__MIPS__) || defined(MIPS) || defined(_MIPS_)
    /*
     * MIPS, bi-endian, so raise error if endianness is not configured
     */
#   undef PJ_M_MIPS
#   define PJ_M_MIPS		1
#   define PJ_M_NAME		"mips"
#   define PJ_HAS_PENTIUM	0
#   if !PJ_IS_LITTLE_ENDIAN && !PJ_IS_BIG_ENDIAN
#   	error Endianness must be declared for this processor
#   endif


#elif defined (PJ_M_SPARC) || defined( 	__sparc__) || defined(__sparc)
    /*
     * Sun Sparc, big endian
     */
#   undef PJ_M_SPARC
#   define PJ_M_SPARC		1
#   define PJ_M_NAME		"sparc"
#   define PJ_HAS_PENTIUM	0
#   define PJ_IS_LITTLE_ENDIAN	0
#   define PJ_IS_BIG_ENDIAN	1

#elif defined(ARM) || defined(_ARM_) ||  defined(__arm__) || defined(_M_ARM)
#   define PJ_HAS_PENTIUM	0
    /*
     * ARM, bi-endian, so raise error if endianness is not configured
     */
#   if !PJ_IS_LITTLE_ENDIAN && !PJ_IS_BIG_ENDIAN
#   	error Endianness must be declared for this processor
#   endif
#   if defined (PJ_M_ARMV7) || defined(ARMV7)
#	undef PJ_M_ARMV7
#	define PJ_M_ARM7		1
#	define PJ_M_NAME		"armv7"
#   elif defined (PJ_M_ARMV4) || defined(ARMV4)
#	undef PJ_M_ARMV4
#	define PJ_M_ARMV4		1
#	define PJ_M_NAME		"armv4"
#   endif 

#elif defined (PJ_M_POWERPC) || defined(__powerpc) || defined(__powerpc__) || \
	defined(__POWERPC__) || defined(__ppc__) || defined(_M_PPC) || \
	defined(_ARCH_PPC)
    /*
     * PowerPC, bi-endian, so raise error if endianness is not configured
     */
#   undef PJ_M_POWERPC
#   define PJ_M_POWERPC		1
#   define PJ_M_NAME		"powerpc"
#   define PJ_HAS_PENTIUM	0
#   if !PJ_IS_LITTLE_ENDIAN && !PJ_IS_BIG_ENDIAN
#   	error Endianness must be declared for this processor
#   endif

#elif defined (PJ_M_NIOS2) || defined(__nios2) || defined(__nios2__) || \
      defined(__NIOS2__) || defined(__M_NIOS2) || defined(_ARCH_NIOS2)
    /*
     * Nios2, little endian
     */
#   undef PJ_M_NIOS2
#   define PJ_M_NIOS2		1
#   define PJ_M_NAME		"nios2"
#   define PJ_HAS_PENTIUM	0
#   define PJ_IS_LITTLE_ENDIAN	1
#   define PJ_IS_BIG_ENDIAN	0
		
#else
#   error "Please specify target machine."
#endif

/* 包含 size_t 定义 */
#include <pj/compat/size_t.h>

/* 包括站点/用户特定的配置以控制 PJLIB 功能。
 * 你必须自己创建这个文件！！
 */
#include <pj/config_site.h>

/********************************************************************
 * PJLIB 特性
 */

/* Overrides for DOXYGEN */
#ifdef DOXYGEN
#   undef PJ_FUNCTIONS_ARE_INLINED
#   undef PJ_HAS_FLOATING_POINT
#   undef PJ_LOG_MAX_LEVEL
#   undef PJ_LOG_MAX_SIZE
#   undef PJ_LOG_USE_STACK_BUFFER
#   undef PJ_TERM_HAS_COLOR
#   undef PJ_POOL_DEBUG
#   undef PJ_HAS_TCP
#   undef PJ_MAX_HOSTNAME
#   undef PJ_IOQUEUE_MAX_HANDLES
#   undef FD_SETSIZE
#   undef PJ_HAS_SEMAPHORE
#   undef PJ_HAS_EVENT_OBJ
#   undef PJ_ENABLE_EXTRA_CHECK
#   undef PJ_EXCEPTION_USE_WIN32_SEH
#   undef PJ_HAS_ERROR_STRING

#   define PJ_HAS_IPV6	1
#endif

/**
 * @defgroup pj_config 构建配置
 * @{
 *
 * 本节包含可以在 PJLIB 构建过程中设置的宏，以控制库的各个方面
 * 注意：此页中的值不一定反映为生成过程中的宏值
 */

/**
 * 如果此宏设置为 1，它将启用库中的某些调试检查
 * 默认值：等于（不是 NDEBUG）
 */
#ifndef PJ_DEBUG
#  ifndef NDEBUG
#    define PJ_DEBUG		    1
#  else
#    define PJ_DEBUG		    0
#  endif
#endif

/**
 * 启用此宏可激活与互斥/信号量相关事件的日志记录
 * 这对于解决诸如死锁之类的并发问题很有用。此外，还应该在日志装饰中添加 PJ_LOG_HAS_THREAD_ID 标志，以帮助解决问题。
 *
 * 默认值：0
 */
#ifndef PJ_DEBUG_MUTEX
#   define PJ_DEBUG_MUTEX	    0
#endif

/**
 * 以内联方式展开 *_i.h头文件中的函数
 *
 * 默认值：0
 */
#ifndef PJ_FUNCTIONS_ARE_INLINED
#  define PJ_FUNCTIONS_ARE_INLINED  0
#endif

/**
 * 使用库中的浮点计算
 *
 * 默认值：1
 */
#ifndef PJ_HAS_FLOATING_POINT
#  define PJ_HAS_FLOATING_POINT	    1
#endif

/**
 * 声明最大日志记录级别/详细程度。数字越小表示重要性越高，重要性最高的级别为零。在这个实现中，最不重要的级别是5，
 * 但是可以通过提供适当的实现来扩展它
 * 级别约定：
 * 	-0:致命错误
 * 	-1:错误
 * 	-2:警告
 * 	-3:信息
 * 	-4:调试
 * 	-5:跟踪
 * 	-6:更详细的跟踪
 *
 * 	默认值：4
 */
#ifndef PJ_LOG_MAX_LEVEL
#  define PJ_LOG_MAX_LEVEL   5
#endif

/**
 * 每次调用 PJ_LOG（）都可以发送到输出设备的最大消息大小。如果邮件大小大于此值，则会被剪切
 * 这可能会影响堆栈的使用，具体取决于是否设置了 PJ_LOG_USE_STACK_BUFFER 标志
 *
 * 默认值：4000
 *
 */
#ifndef PJ_LOG_MAX_SIZE
#  define PJ_LOG_MAX_SIZE	    4000
#endif

/**
 * 日志缓冲区
 * 日志是否从堆栈中获取缓冲区(默认为“是”）。
 * 如果将该值设置为NO，那么将从静态缓冲区获取缓冲区，这将使日志函数不可重入。
 *
 * 默认值：1
 */
#ifndef PJ_LOG_USE_STACK_BUFFER
#  define PJ_LOG_USE_STACK_BUFFER   1
#endif

/**
 * 启用日志缩进功能。
 *
 * 默认值: 1
 */
#ifndef PJ_LOG_ENABLE_INDENT
#   define PJ_LOG_ENABLE_INDENT        1
#endif

/**
 * 每次调用pj_log_push_indent() 时要放入的 PJ_LOG_INDENT_CHAR数
 *
 * 默认值: 1
 */
#ifndef PJ_LOG_INDENT_SIZE
#   define PJ_LOG_INDENT_SIZE        1
#endif

/**
 * 日志缩进字符。
 *
 * 默认值: 空格
 */
#ifndef PJ_LOG_INDENT_CHAR
#   define PJ_LOG_INDENT_CHAR	    '.'
#endif

/**
 * 日志发送器宽度
 *
 * 默认值: 22 (64位机器), 其他 14
 */
#ifndef PJ_LOG_SENDER_WIDTH
#   if PJ_HAS_STDINT_H
#       include <stdint.h>
#       if (UINTPTR_MAX == 0xffffffffffffffff)
#           define PJ_LOG_SENDER_WIDTH  22
#       else
#           define PJ_LOG_SENDER_WIDTH  14
#       endif
#   else
#       define PJ_LOG_SENDER_WIDTH  14
#   endif
#endif

/**
 * 日志线程名称宽度
 *
 * 默认值: 12
 */
#ifndef PJ_LOG_THREAD_WIDTH
#   define PJ_LOG_THREAD_WIDTH	    12
#endif

/**
 * 彩色终端（用于记录等）
 *
 * 默认值: 1
 */
#ifndef PJ_TERM_HAS_COLOR
#  define PJ_TERM_HAS_COLOR	    1
#endif


/**
 * 将此标志设置为非零以启用对池操作的各种检查。设置此标志时，必须在应用程序中启用断言。
 * 这将减慢池的创建和销毁，并将增加几个字节的开销，所以应用程序通常希望在发布版本中禁用此功能
 *
 * 默认值: 0
 */
#ifndef PJ_SAFE_POOL
#   define PJ_SAFE_POOL		    0
#endif


/**
 * 如果使用了池调试，那么池中的每个内存分配都将调用malloc()，当内存块被销毁时，池将释放所有内存块。
 * 当使用诸如 Rational Purify（软件纠错工具） 之类的内存验证程序时，这种方法效果更好
 *
 * 默认值: 0
 */
#ifndef PJ_POOL_DEBUG
#  define PJ_POOL_DEBUG		    0
#endif


/**
 * 启用计时器堆调试工具。启用此选项后，应用程序可以调用 pj_timer_heap_dump() 来显示计时器堆的内容以及调度计时器项的源位置。
 * 更多信息详见：https://trac.pjsip.org/repos/ticket/1527
 *
 * 默认值: 0
 */
#ifndef PJ_TIMER_DEBUG
#  define PJ_TIMER_DEBUG	    0
#endif


/**
 * 将其设置为 1 以启用对组锁的调试。默认值：0
 */
#ifndef PJ_GRP_LOCK_DEBUG
#  define PJ_GRP_LOCK_DEBUG	0
#endif


/**
 * 在 pj_thread_create() 中将此参数指定为堆栈大小参数，以指定线程应使用当前平台的默认堆栈大小
 *
 * 默认值: 8192
 */
#ifndef PJ_THREAD_DEFAULT_STACK_SIZE 
#  define PJ_THREAD_DEFAULT_STACK_SIZE    8192
#endif


/**
 * 指定是否启用 PJ_CHECK_STACK() 宏来检查堆栈的健全性。操作系统实现可以检查没有发生堆栈溢出，
 * 还可以收集有关堆栈使用情况的统计信息。注意，这将增加库的占用空间，因为它跟踪每个函数的文件名和行号
 */
#ifndef PJ_OS_HAS_CHECK_STACK
#	define PJ_OS_HAS_CHECK_STACK		0
#endif

/**
 * 我们有备用池实现吗？
 *
 * Default: 0
 */
#ifndef PJ_HAS_POOL_ALT_API
#   define PJ_HAS_POOL_ALT_API	    PJ_POOL_DEBUG
#endif


/**
 * 库中支持TCP。禁用TCP将略微减少占用空间（约6KB）
 *
 * 默认值: 1
 */
#ifndef PJ_HAS_TCP
#  define PJ_HAS_TCP		    1
#endif

/**
 * 在库中支持IPv6。如果禁用此支持，一些与IPv6相关的函数将返回PJ_EIPV6NOTSUP
 *
 * 默认值: 0 (现在暂不支持)
 */
#ifndef PJ_HAS_IPV6
#  define PJ_HAS_IPV6		    0
#endif

 /**
 *  最大主机名长度
  * 库有时需要将地址复制到堆栈缓冲区；这里的值影响堆栈的使用
 *
 * 默认值: 128
 */
#ifndef PJ_MAX_HOSTNAME
#  define PJ_MAX_HOSTNAME	    (128)
#endif

/**
 * activesock 停止调用下一个 ioqueue accept 之前 accept() 操作的最大连续相同错误数
 *
 * 默认值: 50
 */
#ifndef PJ_ACTIVESOCK_MAX_CONSECUTIVE_ACCEPT_ERROR
#   define PJ_ACTIVESOCK_MAX_CONSECUTIVE_ACCEPT_ERROR 50
#endif

/**
 * 用于声明单个IO队列框架可以支持的最大句柄的常量。这个常量可能与底层的 I/O 队列实现无关，但是开发人员仍然应该注意这个常量，
 * 以确保在底层实现发生更改时程序不会中断
 */
#ifndef PJ_IOQUEUE_MAX_HANDLES
#   define PJ_IOQUEUE_MAX_HANDLES	(64)
#endif


/**
 *
 * 如果 PJ_IOQUEUE_HAS_SAFE_UNREG 宏被定义，那么 ioqueue 将通过对每个句柄使用引用计数器来确保句柄注销操作的线程安全。
 * 此外，ioqueue 将根据 ioqueue 创建期间指定的最大句柄数，为句柄预先分配内存。
 * 所有应用程序通常都希望启用此功能，但您可以在以下情况下禁用此功能：
 *  -没有对所有ioqueue进行动态注销。
 *  -没有线程，或者没有抢先多任务
 *
 * 默认值: 1
 */
#ifndef PJ_IOQUEUE_HAS_SAFE_UNREG
#   define PJ_IOQUEUE_HAS_SAFE_UNREG	1
#endif


/**
 * 注册到 ioqueue 的套接字/句柄的默认并发设置
 * 这控制是否允许 ioqueue 并发/并行地调用键的回调。默认值为 yes，这意味着如果有多个挂起的操作同时完成，则多个线程可能同时调用键的回调。
 * 这通常会促进应用程序良好的可伸缩性，但会牺牲管理并发访问的复杂性。
 *
 * 有关详细信息，请参阅ioqueue文档
 */
#ifndef PJ_IOQUEUE_DEFAULT_ALLOW_CONCURRENCY
#   define PJ_IOQUEUE_DEFAULT_ALLOW_CONCURRENCY   1
#endif


/*
 * 健全性检查：
 *  如果不允许ioqueue并发，则必须启用 PJ_IOQUEUE_HAS_SAFE_UNREG
 */
#if (PJ_IOQUEUE_DEFAULT_ALLOW_CONCURRENCY==0) && (PJ_IOQUEUE_HAS_SAFE_UNREG==0)
#   error PJ_IOQUEUE_HAS_SAFE_UNREG must be enabled if ioqueue concurrency \
	  is disabled
#endif


/**
 * 在 IOQUEUE 中配置安全注销（PJ_IOQUEUE_HAS_SAFE_UNREG）时，PJ_IOQUEUE_KEY_FREE_DELAY 宏指定 IOQUEUE 密钥在可重用之前保持关闭状态的时间。
 * 该值以毫秒为单位
 *
 * 默认值: 500 msec.
 */
#ifndef PJ_IOQUEUE_KEY_FREE_DELAY
#   define PJ_IOQUEUE_KEY_FREE_DELAY	500
#endif


/**
 * 确定 FD_SETSIZE是否可更改/可设置。如果是这样，那么我们将其设置为 PJ_IOQUEUE_MAX_HANDLES。目前我们通过检查 Winsock来检测这一点
 */
#ifndef PJ_FD_SETSIZE_SETABLE
#   if (defined(PJ_HAS_WINSOCK_H) && PJ_HAS_WINSOCK_H!=0) || \
       (defined(PJ_HAS_WINSOCK2_H) && PJ_HAS_WINSOCK2_H!=0)
#	define PJ_FD_SETSIZE_SETABLE	1
#   else
#	define PJ_FD_SETSIZE_SETABLE	0
#   endif
#endif

/**
 * 重写 FD_SETSIZE，使其在整个库中保持一致。
 * 只有当我们检测到 FD_SETSIZE 是可变的时，我们才会这样做。如果 FD_SETSIZE 不可设置，
 * 则 PJ_IOQUEUE_MAX_HANDLES 必须设置为低于 FD_SETSIZE 的值
 */
#if PJ_FD_SETSIZE_SETABLE
    /* Only override FD_SETSIZE if the value has not been set */
#   ifndef FD_SETSIZE
#	define FD_SETSIZE		PJ_IOQUEUE_MAX_HANDLES
#   endif
#else
    /*
     * 当 FD_SETSIZE 不可更改时，检查 PJ_IOQUEUE_MAX_HANDLES 是否低于 FD_SETSIZE 值。
     * 更新：并不是所有 ioqueue 后端都需要这样做（比如 epoll ），所以这个检查将在 ioqueue 实现本身上完成，比如 ioqueue select
     */
/*
#   ifdef FD_SETSIZE
#	if PJ_IOQUEUE_MAX_HANDLES > FD_SETSIZE
#	    error "PJ_IOQUEUE_MAX_HANDLES is greater than FD_SETSIZE"
#	endif
#   endif
*/
#endif


/**
 * 指定 pj_enum_ip_interface() 函数是否应排除环回接口
 *
 * 默认值: 1
 */
#ifndef PJ_IP_HELPER_IGNORE_LOOPBACK_IF
#   define PJ_IP_HELPER_IGNORE_LOOPBACK_IF	1
#endif


/**
 * 是否有信号量功能
 *
 * 默认值: 1
 */
#ifndef PJ_HAS_SEMAPHORE
#  define PJ_HAS_SEMAPHORE	    1
#endif


/**
 * 事件对象(应用于同步, e.g. in Win32)
 *
 * 默认值: 1
 */
#ifndef PJ_HAS_EVENT_OBJ
#  define PJ_HAS_EVENT_OBJ	    1
#endif


/**
 * 最大文件名长度
 */
#ifndef PJ_MAXPATH
#   define PJ_MAXPATH		    260
#endif


/**
 * 启用库的额外检查。
 * 如果启用此宏，PJ_ASSERT_RETURN 宏将扩展到运行时检查。如果禁用此宏，PJ_ASSERT_RETURN 将直接计算为 pj_assert()
 * 如果将无效值（例如NULL）传递给库，则可以禁用此宏以减小大小，但有发生崩溃的风险
 *
 * 默认值: 1
 */
#ifndef PJ_ENABLE_EXTRA_CHECK
#   define PJ_ENABLE_EXTRA_CHECK    1
#endif


/**
 * 使用 pj_exception_id_alloc() 为异常启用名称注册
 * 如果启用此功能，则库将跟踪与应用程序通过 pj_exception_id_alloc() 请求的每个异常ID关联的名称
 * 禁用此宏将使 代码和.bss大小减少一点点。
 * 另请参见 PJ_MAX_EXCEPTION_ID
 *
 * 默认值: 1
 */
#ifndef PJ_HAS_EXCEPTION_NAMES
#   define PJ_HAS_EXCEPTION_NAMES   1
#endif

/**
 * 使用 pj_exception_id_alloc()可以请求的唯一异常id的最大数目。对于每个条目，将在 .bss段中分配一个小记录
 *
 * 默认值: 16
 */
#ifndef PJ_MAX_EXCEPTION_ID
#   define PJ_MAX_EXCEPTION_ID      16
#endif

/**
 * 我们是否应该对 PJLIB异常使用 Windows 结构化异常处理（SEH）
 *
 * 默认值: 0
 */
#ifndef PJ_EXCEPTION_USE_WIN32_SEH
#  define PJ_EXCEPTION_USE_WIN32_SEH 0
#endif

/**
 * 我们是否应该尝试使用 Pentium 的 rdtsc 作为高分辨率时间戳
 *
 * 默认值: 0
 */
#ifndef PJ_TIMESTAMP_USE_RDTSC
#   define PJ_TIMESTAMP_USE_RDTSC   0
#endif

/**
 * 本机平台错误是正数吗？
 * 默认值：1（是）
 */
#ifndef PJ_NATIVE_ERR_POSITIVE
#   define PJ_NATIVE_ERR_POSITIVE   1
#endif
 
/**
 * 在库中包含错误消息字符串（pj_strerror()）。
 * 这是非常可取的！
 *
 * 默认值：1
 */
#ifndef PJ_HAS_ERROR_STRING
#   define PJ_HAS_ERROR_STRING	    1
#endif


/**
 * 包括 pj_stricmp_alnum()和 pj_strnicmp_alnum()，即用于比较 alnum 字符串的自定义函数。
 * 在某些系统上，它们比 stricmp/strcasecmp 更快，但在其他系统上可能较慢。
 * 禁用时，pjlib将回退到stricmp/strnicmp
 *
 * 默认值：0
 */
#ifndef PJ_HAS_STRICMP_ALNUM
#   define PJ_HAS_STRICMP_ALNUM	    0
#endif


/*
 * QoS后端实现的类型
 */

/** 
 * 虚拟 QoS 后端实现，将始终在所有API上返回错误
 */
#define PJ_QOS_DUMMY	    1

/** 基于setsockopt（IP_TOS）的QoS后端 */
#define PJ_QOS_BSD	    2

/** Windows Mobile 6的 QoS后端 */
#define PJ_QOS_WM	    3

/** Symbian 的QoS 后端  */
#define PJ_QOS_SYMBIAN	    4

/** Darwin的QoS 后端 */
#define PJ_QOS_DARWIN	    5

/**
 * 强制对某些平台使用某些QoS后端API
 */
#ifndef PJ_QOS_IMPLEMENTATION
#   if defined(PJ_WIN32_WINCE) && PJ_WIN32_WINCE && _WIN32_WCE >= 0x502
	/* Windows Mobile 6 or later */
#	define PJ_QOS_IMPLEMENTATION    PJ_QOS_WM
#   elif defined(PJ_DARWINOS)
	/* Darwin OS (e.g: iOS, MacOS, tvOS) */
#	define PJ_QOS_IMPLEMENTATION    PJ_QOS_DARWIN
#   endif
#endif


/**
 * 启用安全套接字。对于大多数平台，这是使用 OpenSSL 或 GnuTLS 实现的，因此需要安装其中一个库。对于 Symbian平台，这是使用 CSecureSocket 本机实现的。
 *
 * 默认值：0（暂时）
 */
#ifndef PJ_HAS_SSL_SOCK
#  define PJ_HAS_SSL_SOCK	    0
#endif


/*
 * 安全套接字实现。
 * 在 PJ_SSL_SOCK_IMP 中选择其中一个实现
 */
#define PJ_SSL_SOCK_IMP_NONE 	    0	/**< 禁用 SSL socket.    */
#define PJ_SSL_SOCK_IMP_OPENSSL	    1	/**< 使用 OpenSSL.	    */
#define PJ_SSL_SOCK_IMP_GNUTLS      2	/**< 使用 GnuTLS.	    */


/**
 * 选择要使用的 SSL套接字实现。目前 pjlib支持使用 OPENSSL 的 PJ_SSL_SOCK_IMP_OPENSSL和使用 GNUTLS 的 PJ_SSL_SOCK_IMP_GNUTLS。
 * 将此设置为 PJ_SSL_SOCK_IMP_NONE 将禁用安全套接字。
 *
 * 默认值为 PJ_SSL_SOCK_IMP_NONE如果PJ_HAS_SSL_SOCK未设置
 * 否则为PJ_SSL_SOCK_IMP_OPENSSL
 */
#ifndef PJ_SSL_SOCK_IMP
#   if PJ_HAS_SSL_SOCK==0
#	define PJ_SSL_SOCK_IMP		    PJ_SSL_SOCK_IMP_NONE
#   else
#	define PJ_SSL_SOCK_IMP		    PJ_SSL_SOCK_IMP_OPENSSL
#   endif
#endif


/**
 * 定义安全套接字支持的最大密码数。
 *
 * 默认值：256
 */
#ifndef PJ_SSL_SOCK_MAX_CIPHERS
#  define PJ_SSL_SOCK_MAX_CIPHERS   256
#endif


/**
 * 指定应设置为可用 SSL_CIPHERs 列表的内容。例如，将此设置为 "DEFAULT" 以使用默认密码列表（注意：PJSIP release 2.4 和使用此"DEFAULT"设置之前）
 * 默认值："HIGH:-COMPLEMENTOFDEFAULT"
 */
#ifndef PJ_SSL_SOCK_OSSL_CIPHERS
#  define PJ_SSL_SOCK_OSSL_CIPHERS   "HIGH:-COMPLEMENTOFDEFAULT"
#endif


/**
 * 定义安全套接字支持的最大曲线数。
 * 默认值：32
 */
#ifndef PJ_SSL_SOCK_MAX_CURVES
#  define PJ_SSL_SOCK_MAX_CURVES   32
#endif


/**
 * 对Win32平台上的UDP套接字禁用 WSAECONNRESET错误。
 * 详见 ：https://trac.pjsip.org/repos/ticket/1197.
 *
 * 默认值：1
 */
#ifndef PJ_SOCK_DISABLE_WSAECONNRESET
#   define PJ_SOCK_DISABLE_WSAECONNRESET    1
#endif


/**
 * pj_sockopt_params 中套接字选项的最大数目。
 *
 * 默认值：4
 */
#ifndef PJ_MAX_SOCKOPT_PARAMS
#   define PJ_MAX_SOCKOPT_PARAMS	    4
#endif



/** @} */

/********************************************************************
 * 常规宏
 */

/**
 * @defgroup pj_dll_target 构建动态链接库（DLL/DSO）
 * @ingroup pj_config
 * @{
 *
 * 这些库支持为 Symbian ABIv2 目标生成动态链接库（.dso/Symbian术语中的动态共享对象文件）。
 * 类似的过程可以应用于win32dll，但需要做一些修改
 *
 *
 * 根据平台的不同，生成动态库可能需要以下步骤：
 *  -创建（visualstudio）项目以生成DLL输出。PJLIB不提供现成的项目文件来生成DLL，所以您需要自己创建这些项目。
 *      对于Symbian，MMP文件已设置为需要它们的目标生成DSO文件。
 *  -在（visualstudio）项目中，需要声明一些宏，以便向符号声明和定义添加适当的修饰符。有关这些宏的信息，请参见下面的宏部分。
 *      对于Symbian，这些都是由MMP文件处理的。
 *  -某些生成系统需要在创建DLL时指定.DEF文件。对于Symbian，.DEF文件包含在 pjlib 发行版的 pjlib/build.Symbian 目录中。
 *      这些DEF文件是从 Mingw 中的这个目录运行 /makedef.sh all 创建的
 *
 * 与生成DLL/DSO文件相关的宏：
 * -对于支持动态链接库生成的平台，它必须声明包含要添加到符号定义的前缀的 PJ_EXPORT_SPECIFIER 宏，才能在DLL/DSO中导出此符号。
 *      例如，在Win32/Visual Studio上，此宏的值是 __declspec（dllexport），而对于ARM ABIv2/Symbian，该值是 EXPORT_C
 * -对于支持与动态链接库链接的平台，它必须声明包含要添加到符号声明的前缀的 PJ_IMPORT_SPECIFIER 宏，才能从DLL/DSO导入此符号。
 *      例如，在Win32/Visual Studio上，此宏的值是 __declspec（dllimport），而对于ARM ABIv2/Symbian，该值是 IMPORT_C
 * -如果 pjlib 未声明上述 PJ_EXPORT_SPECIFIER 和 PJ_EXPORT_SPECIFIER 宏，则它们都可以在 config_site.h 中声明
 * -当 PJLIB 构建为 DLL/DSO 时，必须声明 PJ_DLL 和 PJ_EXPORTING 宏，以便将 PJ_EXPORT_SPECIFIER 修饰符添加到函数定义中
 * -当应用程序要与 PJLIB 动态链接时，则必须在使用/包括 PJLIB 头时声明 PJ_DLL 宏，以便将 PJ_IMPORT_SPECIFIER 修饰符正确地添加到符号声明中
 *
 * 如果未声明 PJ_DLL 宏，则假定为静态链接
 *
 * 例如，以下是在 Windows/Win32 上使用 Visual Studio生成DLL的一些设置：
 *  -创建VisualStudio项目以生成 DLL。添加适当的项目依赖项以避免链接错误
 *  -在项目中，声明 PJ_DLL 和 PJ_EXPORTING 宏
 *  -在 config_site.h 中声明这些宏：
 \verbatim
	#define PJ_EXPORT_SPECIFIER  __declspec(dllexport)
	#define PJ_IMPORT_SPECIFIER  __declspec(dllimport)
 \endverbatim
    在应用程序（与DLL链接）项目中，在宏声明中添加 PJ_DLL
 */

/** @} */

/**
 * @defgroup pj_config 生成配置
 * @{
 */

/**
 * @def PJ_INLINE(type)
 * @param 键入函数的返回类型
 * 将函数展开为inline
 */
#define PJ_INLINE(type)	  PJ_INLINE_SPECIFIER type

/**
 * 当 PJLIB 构建为动态库时，此宏声明要添加到符号声明中的平台/编译器特定说明符前缀，以导出符号。
 * 如果平台支持构建动态库目标，则此宏应该由特定于平台的标头添加
 */
#ifndef PJ_EXPORT_DECL_SPECIFIER
#   define PJ_EXPORT_DECL_SPECIFIER
#endif


/**
 * 当 PJLIB 构建为动态库时，此宏声明要添加到符号定义的平台/编译器特定说明符前缀，以导出符号。
 * 如果平台支持构建动态库目标，则此宏应该由特定于平台的标头添加
 */
#ifndef PJ_EXPORT_DEF_SPECIFIER
#   define PJ_EXPORT_DEF_SPECIFIER
#endif


/**
 * 此宏声明要添加到符号声明以导入符号的平台/编译器特定说明符前缀。
 * 如果平台支持构建动态库目标，则此宏应该由特定于平台的标头添加
 */
#ifndef PJ_IMPORT_DECL_SPECIFIER
#   define PJ_IMPORT_DECL_SPECIFIER
#endif


/**
 * 此宏已被弃用。它将评估为零
 */
#ifndef PJ_EXPORT_SYMBOL
#   define PJ_EXPORT_SYMBOL(x)
#endif


/**
 * @def PJ_DECL(type)
 * @param type 函数的返回类型
 * 声明函数
 */
#if defined(PJ_DLL)
#   if defined(PJ_EXPORTING)
#	define PJ_DECL(type)	    PJ_EXPORT_DECL_SPECIFIER type
#   else
#	define PJ_DECL(type)	    PJ_IMPORT_DECL_SPECIFIER type
#   endif
#elif !defined(PJ_DECL)
#   if defined(__cplusplus)
#	define PJ_DECL(type)	    type
#   else
#	define PJ_DECL(type)	    extern type
#   endif
#endif


/**
 * @def PJ_DEF(type)
 * @param type 函数的返回类型
 * 定义一个函数
 */
#if defined(PJ_DLL) && defined(PJ_EXPORTING)
#   define PJ_DEF(type)		    PJ_EXPORT_DEF_SPECIFIER type
#elif !defined(PJ_DEF)
#   define PJ_DEF(type)		    type
#endif


/**
 * @def PJ_DECL_NO_RETURN(type)
 * @param type 函数的返回类型
 * 声明一个没有返回值的函数。
 */
/**
 * @def PJ_IDECL_NO_RETURN(type)
 * @param type 函数的返回类型
 * 声明一个没有返回值的内联函数
 */
/**
 * @def PJ_BEGIN_DECL
 * 在头文件中标记声明节的开头
 */
/**
 * @def PJ_END_DECL
 * 在头文件中标记声明节的结尾
 */
#ifdef __cplusplus
#  define PJ_DECL_NO_RETURN(type)   PJ_DECL(type) PJ_NORETURN
#  define PJ_IDECL_NO_RETURN(type)  PJ_INLINE(type) PJ_NORETURN
#  define PJ_BEGIN_DECL		    extern "C" {
#  define PJ_END_DECL		    }
#else
#  define PJ_DECL_NO_RETURN(type)   PJ_NORETURN PJ_DECL(type)
#  define PJ_IDECL_NO_RETURN(type)  PJ_NORETURN PJ_INLINE(type)
#  define PJ_BEGIN_DECL
#  define PJ_END_DECL
#endif



/**
 * @def PJ_DECL_DATA(type)
 * @param type 数据类型
 * 声明全局变量
 */ 
#if defined(PJ_DLL)
#   if defined(PJ_EXPORTING)
#	define PJ_DECL_DATA(type)   PJ_EXPORT_DECL_SPECIFIER extern type
#   else
#	define PJ_DECL_DATA(type)   PJ_IMPORT_DECL_SPECIFIER extern type
#   endif
#elif !defined(PJ_DECL_DATA)
#   define PJ_DECL_DATA(type)	    extern type
#endif


/**
 * @def PJ_DEF_DATA(type)
 * @param type 数据类型
 * 定义一个全局变量
 */ 
#if defined(PJ_DLL) && defined(PJ_EXPORTING)
#   define PJ_DEF_DATA(type)	    PJ_EXPORT_DEF_SPECIFIER type
#elif !defined(PJ_DEF_DATA)
#   define PJ_DEF_DATA(type)	    type
#endif


/**
 * @def PJ_IDECL(type)
 * @param type  函数返回类型
 * 声明一个可以扩展为内联的函数
 */
/**
 * @def PJ_IDEF(type)
 * @param type  函数返回类型
 * 定义一个可以扩展为内联的函数
 */

#if PJ_FUNCTIONS_ARE_INLINED
#  define PJ_IDECL(type)  PJ_INLINE(type)
#  define PJ_IDEF(type)   PJ_INLINE(type)
#else
#  define PJ_IDECL(type)  PJ_DECL(type)
#  define PJ_IDEF(type)   PJ_DEF(type)
#endif


/**
 * @def PJ_UNUSED_ARG(arg)
 * @param arg   参数名称
 * PJ_UNUSED_ARG 防止对函数中未使用的参数发出警告
 */
#define PJ_UNUSED_ARG(arg)  (void)arg

/**
 * @def PJ_TODO(id)
 * @param id    将作为TODO消息打印的标识符
 * PJ_TODO 宏在编译期间将TODO消息显示为警告
 * Example: PJ_TODO(CLEAN_UP_ERROR);
 */
#ifndef PJ_TODO
#  define PJ_TODO(id)	    TODO___##id:
#endif

/**
 * 通过在战略位置休眠线程来模拟竞争条件。
 * 默认值：否
 */
#ifndef PJ_RACE_ME
#  define PJ_RACE_ME(x)
#endif

/**
 * 函数属性来通知函数可能引发异常
 *
 * @param x     异常列表，用括号括起来
 */
#define __pj_throw__(x)

/** @} */

/********************************************************************
 * 健全性检查
 */
#ifndef PJ_HAS_HIGH_RES_TIMER
#  error "PJ_HAS_HIGH_RES_TIMER is not defined!"
#endif

#if !defined(PJ_HAS_PENTIUM)
#  error "PJ_HAS_PENTIUM is not defined!"
#endif

#if !defined(PJ_IS_LITTLE_ENDIAN)
#  error "PJ_IS_LITTLE_ENDIAN is not defined!"
#endif

#if !defined(PJ_IS_BIG_ENDIAN)
#  error "PJ_IS_BIG_ENDIAN is not defined!"
#endif

#if !defined(PJ_EMULATE_RWMUTEX)
#  error "PJ_EMULATE_RWMUTEX should be defined in compat/os_xx.h"
#endif

#if !defined(PJ_THREAD_SET_STACK_SIZE)
#  error "PJ_THREAD_SET_STACK_SIZE should be defined in compat/os_xx.h"
#endif

#if !defined(PJ_THREAD_ALLOCATE_STACK)
#  error "PJ_THREAD_ALLOCATE_STACK should be defined in compat/os_xx.h"
#endif

PJ_BEGIN_DECL

/** PJLIB 主版本号 */
#define PJ_VERSION_NUM_MAJOR	2

/** PJLIB 次版本号 */
#define PJ_VERSION_NUM_MINOR	8

/** PJLIB 修订版本号 */
#define PJ_VERSION_NUM_REV      0

/**
 * 版本的附加后缀（例如 "-trunk"），或对于web发布版本为空
 */
#define PJ_VERSION_NUM_EXTRA	""

/**
 * PJLIB 版本号由以下格式的三个字节组成：
 *  0xMMIIRR00，
 *      其中MM:主要编号，
 *      II:次要编号，
 *      RR:修订编号，
 *      00:目前始终为零
 */
#define PJ_VERSION_NUM	((PJ_VERSION_NUM_MAJOR << 24) |	\
			 (PJ_VERSION_NUM_MINOR << 16) | \
			 (PJ_VERSION_NUM_REV << 8))

/**
 * PJLIB 版本字符串常量 @see pj_get_version()
 */
PJ_DECL_DATA(const char*) PJ_VERSION;

/**
 * Get PJLIB 版本字符串
 *
 * @return #PJ_VERSION 常量
 */
PJ_DECL(const char*) pj_get_version(void);

/**
 * 将配置转储到日志，详细程度等于 info（3）
 */
PJ_DECL(void) pj_dump_config(void);

PJ_END_DECL


#endif	/* __PJ_CONFIG_H__ */

