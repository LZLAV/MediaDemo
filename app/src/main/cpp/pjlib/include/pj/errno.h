/**
 * 错误子系统
 */
#ifndef __PJ_ERRNO_H__
#define __PJ_ERRNO_H__

/**
 * @file errno.h
 * @brief PJLIB 错误子系统
 */
#include <pj/types.h>
#include <pj/compat/errno.h>
#include <stdarg.h>

PJ_BEGIN_DECL

/**
 * @defgroup pj_errno 错误子系统
 * @{
 *
 * PJLIB 错误子系统是一个框架，它将所有组件产生的所有错误代码统一到一个错误空间中，并提供一组统一的 api来访问它们。
 * 在这个框架中，任何错误代码都被编码为 pj_status_t 值。该框架是可扩展的，应用程序可以注册新的错误空间以供框架识别
 *
 * @section pj_errno_retval 返回值
 *
 * 返回 pj_status_t 的所有函数返回 PJ_SUCCESS（如果操作成功完成），或非零值表示错误。如果错误来自操作系统，
 * 则使用 PJ_STATUS_FROM_OS() 宏将本机错误代码转换/折叠到 PJLIB 的错误命名空间中。函数在将错误返回给调
 * 用者之前自动执行此操作
 *
 * @section err_services 检索和显示错误消息
 *
 * 框架提供以下API来检索和、或显示错误消息：
 * 	-pj_strerror()：这是基本API 用于检索指定pj_status_t错误代码的错误字符串描述
 * 	-PJ_PERROR()宏：使用类似于 PJ_LOG 的宏格式化错误消息并将其显示到日志中
 * 	-pj_perror()：此函数类似于 PJ_PERROR()，但不同于 PJ_PERROR()，此函数将始终包含在链接进程中。
 * 		由于这个原因，如果应用程序关心可执行文件的大小，最好使用 PJ_PERROR()
 *
 * 应用程序不能将本机错误代码（例如来自 GetLastError() 或errno 等函数的错误代码）传递给需要 pj_status_t 的 PJLIB 函数
 *
 * @section err_extending 扩展错误空间
 *
 * 应用程序可以使用 pj_register_strerror() 注册新的错误空间，以供框架识别。使用从 PJ_ERRNO_START_USER 开始的范围，以避免与现有错误空间冲突
 *
 */

/**
 * 错误消息长度准则
 */
#define PJ_ERR_MSG_SIZE  80

/**
 * PJ_PERROR()标题字符串的缓冲区
 */
#ifndef PJ_PERROR_TITLE_BUF_SIZE
#   define PJ_PERROR_TITLE_BUF_SIZE	120
#endif


/**
 * 获取最后一个平台错误/状态，折叠为 pj_status_t
 * @return	系统依赖错误码，折合为 pj_status_t
 * @remark	此函数获取 errno，或调用 GetLastError() 函数，并将代码从 PJ_STATUS_FROM_OS 转换为pj_status_t,套接字函数不要调用此函数
 * @see	pj_get_netos_error()
 */
PJ_DECL(pj_status_t) pj_get_os_error(void);

/**
 * 设置最后一个错误
 * @param code	pj_status_t
 */
PJ_DECL(void) pj_set_os_error(pj_status_t code);

/**
 * 从套接字操作获取最后一个错误
 * @return	最后一个 socket 错误，折合为 pj_status_t
 */
PJ_DECL(pj_status_t) pj_get_netos_error(void);

/**
 * 设置错误码
 * @param code	pj_status_t.
 */
PJ_DECL(void) pj_set_netos_error(pj_status_t code);


/**
 * 获取指定错误代码的错误消息。消息字符串将以NULL结尾
 *
 * @param statcode  错误码
 * @param buf	    用于保存错误消息字符串的缓冲区
 * @param bufsize   缓冲区的大小
 *
 * @return	    错误消息是以NULL结尾的字符串，用pj_str_t包装
 */
PJ_DECL(pj_str_t) pj_strerror( pj_status_t statcode, 
			       char *buf, pj_size_t bufsize);

/**
 * 实用程序宏，用于将与指定错误代码有关的错误消息打印到日志中。此宏将根据 'title_fmt' 参数构造错误消息标题，
 * 并在标题字符串后添加与错误代码相关的错误字符串。将在标题和错误字符串之间自动添加冒号(':')
 *
 * 此函数类似于 pj_perror() 函数，但其优点是，如果 log level 参数低于 PJ_LOG_MAX_LEVEL 阈值，则可以从链接进程中省略函数调用。
 *
 * 注意，由title_fmt构造的 title 字符串将构建在一个字符串缓冲区上，该缓冲区的大小是 PJ_PERROR_TITLE_BUF_SIZE，通常是从堆栈中分配的。
 * 默认情况下，缓冲区大小很小（大约120个字符）。应用程序必须确保构造的标题字符串不会超过此限制，因为并非所有平台都支持截断该字符串
 *
 * @see pj_perror()
 *
 * @param level	    日志详细级别，有效值为0-6。数字越小表示重要性越高，级别0表示命致错误。只允许数字参数（例如不允许变量）
 * @param arg	    包含类似 'printf' 的参数，包括以下参数：
 * 						-发送方（以NULL结尾的字符串）
 * 						-错误代码（pj_status_t）
 * 						-格式字符串（title_fmt），以及
 * 						-适用于格式字符串的可选变量数
 *
 * Sample:
 * \verbatim
   PJ_PERROR(2, (__FILE__, PJ_EBUSY, "Error making %s", "coffee"));
   \endverbatim
 * @hideinitializer
 */
#define PJ_PERROR(level,arg)	do { \
				    pj_perror_wrapper_##level(arg); \
				} while (0)

/**
 * 将与指定错误代码有关的错误消息打印到日志的实用程序函数。此函数将根据 'title_fmt' 参数构造错误消息标题，
 * 并在标题字符串后添加与错误代码相关的错误字符串。将在标题和错误字符串之间自动添加冒号(':')
 *
 * 与PJ_PERROR()宏不同，此函数将 log_level 的参数作为普通参数，与 PJ_PERROR() 中必须给定数值的情况不同。
 * 但是，此函数将始终链接到可执行文件，这与PJ_PERROR()不同，PJ_PERROR()在级别低于PJ_LOG_MAX_LEVEL 时可以省略
 *
 * 注意，由 title_fmt 构造的 title 字符串将构建在一个字符串缓冲区上，该缓冲区的大小是 PJ_PERROR_TITLE_BUF_SIZE，
 * 通常是从堆栈中分配的。默认情况下，缓冲区大小很小（大约120个字符）。应用程序必须确保构造的标题字符串不会超过此限制，
 * 因为并非所有平台都支持截断该字符串
 *
 * @see PJ_PERROR()
 */
PJ_DECL(void) pj_perror(int log_level, const char *sender, pj_status_t status,
		        const char *title_fmt, ...);


/**
 * 在 pj_register_strerror() 中指定的回调类型
 *
 * @param e	    要查找的错误代码。
 * @param msg	    用于存储错误消息的缓冲区
 * @param max	    缓冲区的长度
 *
 * @return	    错误字符串
 */
typedef pj_str_t (*pj_error_callback)(pj_status_t e, char *msg, pj_size_t max);


/**
 * 为指定的错误空间注册 strerror 消息处理程序。
 * 应用程序可以注册自己的处理程序，为指定的错误代码范围提供错误消息。此处理程序将由 pj_strerror() 调用
 *
 * @param start_code	应在其中调用处理程序以检索错误消息的起始错误代码
 * @param err_space		错误空间的大小。然后，错误代码范围将落在start_code 到start_code+err_space-1 的范围内
 * @param f		当为 pj_strerror() 提供了属于此范围的错误代码时要调用的处理程序
 *
 * @return		成功返回 PJ_SUCCESS，否则为指定的错误码
 *				当错误空间已被其他处理程序占用，或者注册到PJLIB的处理程序太多时，注册可能会失败。
 */
PJ_DECL(pj_status_t) pj_register_strerror(pj_status_t start_code,
					  pj_status_t err_space,
					  pj_error_callback f);

/**
 * @hideinitializer
 * 返回平台操作系统错误代码折叠成 pj_status_t 代码。这是整个库中用于所有PJLIB函数的宏，这些函数从操作系统返回错误。
 * 应用程序可以重写此宏以减小大小（例如，通过将其定义为始终返回PJ_EUNKNOWN）
 *
 * 注：
 * 	无论是否将零作为参数传递，此宏都必须返回非零值。原因是在操作系统不能正确报告错误代码时保护逻辑错误
 *
 * @param os_code   平台操作系统错误代码。此值可以多次计算
 * @return	    平台操作系统错误代码折叠成pj_status_t
 */
#ifndef PJ_RETURN_OS_ERROR
#   define PJ_RETURN_OS_ERROR(os_code)   (os_code ? \
					    PJ_STATUS_FROM_OS(os_code) : -1)
#endif


/**
 * @hideinitializer
 * 将特定于平台的错误折叠到pj_status_t代码中
 *
 * @param e	平台操作系统错误代码。
 * @return	pj_status_t
 * @warning	宏观实施；syserr参数可以多次求值
 */
#if PJ_NATIVE_ERR_POSITIVE
#   define PJ_STATUS_FROM_OS(e) (e == 0 ? PJ_SUCCESS : e + PJ_ERRNO_START_SYS)
#else
#   define PJ_STATUS_FROM_OS(e) (e == 0 ? PJ_SUCCESS : PJ_ERRNO_START_SYS - e)
#endif

/**
 * @hideinitializer
 * 将 pj_status_t 代码折叠回本机平台定义的错误
 *
 * @param e	pj_status_t折叠平台操作系统错误代码
 * @return	pj_os_err_type
 * @warning	宏实现；statcode 参数可以多次求值。如果 statcode 不是由 pj_get_os_error 或 PJ_STATUS_FROM_OS 创建的，则结果未定义
 */
#if PJ_NATIVE_ERR_POSITIVE
#   define PJ_STATUS_TO_OS(e) (e == 0 ? PJ_SUCCESS : e - PJ_ERRNO_START_SYS)
#else
#   define PJ_STATUS_TO_OS(e) (e == 0 ? PJ_SUCCESS : PJ_ERRNO_START_SYS - e)
#endif


/**
 * @defgroup pj_errnum PJLIB's 自身错误代码
 * @ingroup pj_errno
 * @{
 */

/**
 * 使用此宏为错误代码生成错误消息文本，以便它们与其余库一致
 *
 * @param code	错误代码
 * @param msg	错误测试
 */
#ifndef PJ_BUILD_ERR
#   define PJ_BUILD_ERR(code,msg) { code, msg " (" #code ")" }
#endif


/**
 * @hideinitializer
 * 报告了未知错误
 */
#define PJ_EUNKNOWN	    (PJ_ERRNO_START_STATUS + 1)	/* 70001 */
/**
 * @hideinitializer
 * 操作正在挂起，稍后将完成
 */
#define PJ_EPENDING	    (PJ_ERRNO_START_STATUS + 2)	/* 70002 */
/**
 * @hideinitializer
 * 太多连接的 socket
 */
#define PJ_ETOOMANYCONN	    (PJ_ERRNO_START_STATUS + 3)	/* 70003 */
/**
 * @hideinitializer
 * 无效的参数
 */
#define PJ_EINVAL	    (PJ_ERRNO_START_STATUS + 4)	/* 70004 */
/**
 * @hideinitializer
 * 名称太长（例如：主机名太长）
 */
#define PJ_ENAMETOOLONG	    (PJ_ERRNO_START_STATUS + 5)	/* 70005 */
/**
 * @hideinitializer
 * 没有找到
 */
#define PJ_ENOTFOUND	    (PJ_ERRNO_START_STATUS + 6)	/* 70006 */
/**
 * @hideinitializer
 * 内存不足
 */
#define PJ_ENOMEM	    (PJ_ERRNO_START_STATUS + 7)	/* 70007 */
/**
 * @hideinitializer
 * 检测到错误！
 */
#define PJ_EBUG             (PJ_ERRNO_START_STATUS + 8)	/* 70008 */
/**
 * @hideinitializer
 * 操作超时
 */
#define PJ_ETIMEDOUT        (PJ_ERRNO_START_STATUS + 9)	/* 70009 */
/**
 * @hideinitializer
 * 对象太多
 */
#define PJ_ETOOMANY         (PJ_ERRNO_START_STATUS + 10)/* 70010 */
/**
 * @hideinitializer
 * 对象忙
 */
#define PJ_EBUSY            (PJ_ERRNO_START_STATUS + 11)/* 70011 */
/**
 * @hideinitializer
 * 不支持指定的选项
 */
#define PJ_ENOTSUP	    (PJ_ERRNO_START_STATUS + 12)/* 70012 */
/**
 * @hideinitializer
 * 无效操作
 */
#define PJ_EINVALIDOP	    (PJ_ERRNO_START_STATUS + 13)/* 70013 */
/**
 * @hideinitializer
 * 操作被取消
 */
#define PJ_ECANCELLED	    (PJ_ERRNO_START_STATUS + 14)/* 70014 */
/**
 * @hideinitializer
 * 对象已经存在
 */
#define PJ_EEXISTS          (PJ_ERRNO_START_STATUS + 15)/* 70015 */
/**
 * @hideinitializer
 * 文件末尾
 */
#define PJ_EEOF		    (PJ_ERRNO_START_STATUS + 16)/* 70016 */
/**
 * @hideinitializer
 * Size太大
 */
#define PJ_ETOOBIG	    (PJ_ERRNO_START_STATUS + 17)/* 70017 */
/**
 * @hideinitializer
 * gethostbyname（）返回错误时返回的一般错误
 */
#define PJ_ERESOLVE	    (PJ_ERRNO_START_STATUS + 18)/* 70018 */
/**
 * @hideinitializer
 * Size 太小
 */
#define PJ_ETOOSMALL	    (PJ_ERRNO_START_STATUS + 19)/* 70019 */
/**
 * @hideinitializer
 * 忽略
 */
#define PJ_EIGNORED	    (PJ_ERRNO_START_STATUS + 20)/* 70020 */
/**
 * @hideinitializer
 * 不支持 IPv6
 */
#define PJ_EIPV6NOTSUP	    (PJ_ERRNO_START_STATUS + 21)/* 70021 */
/**
 * @hideinitializer
 * 不支持的地址族
 */
#define PJ_EAFNOTSUP	    (PJ_ERRNO_START_STATUS + 22)/* 70022 */
/**
 * @hideinitializer
 * 对象不再存在
 */
#define PJ_EGONE	    (PJ_ERRNO_START_STATUS + 23)/* 70023 */
/**
 * @hideinitializer
 * Socket已经停止
 */
#define PJ_ESOCKETSTOP	    (PJ_ERRNO_START_STATUS + 24)/* 70024 */

/** @} */   /* pj_errnum */

/** @} */   /* pj_errno */


/**
 * PJ_ERRNO_START PJLIB指定的错误起始
 */
#define PJ_ERRNO_START		20000

/**
 * PJ_ERRNO_SPACE_SIZE 是以下某个错误/状态范围内的最大错误数
 */
#define PJ_ERRNO_SPACE_SIZE	50000

/**
 * PJ_ERRNO_START_STATUS 是PJLIB特定状态代码的起始位置。实际上，这个类中的有效错误是70000-119000
 */
#define PJ_ERRNO_START_STATUS	(PJ_ERRNO_START + PJ_ERRNO_SPACE_SIZE)

/**
 * PJ_ERRNO_START_SYS 将特定于平台的错误代码转换为 pj_status_t 值。
 * 实际上，这个类中的有效错误是120000-169000
 */
#define PJ_ERRNO_START_SYS	(PJ_ERRNO_START_STATUS + PJ_ERRNO_SPACE_SIZE)

/**
 * PJ_ERRNO_START_USER 是为使用错误代码和 PJLIB 代码的应用程序保留的。
 * 实际上，这个类中的有效错误是170000-219000
 */
#define PJ_ERRNO_START_USER	(PJ_ERRNO_START_SYS + PJ_ERRNO_SPACE_SIZE)


/*
 * 以下是迄今为止所使用的错误空间列表：
 *  - PJSIP_ERRNO_START		(PJ_ERRNO_START_USER)
 *  - PJMEDIA_ERRNO_START	(PJ_ERRNO_START_USER + PJ_ERRNO_SPACE_SIZE)
 *  - PJSIP_SIMPLE_ERRNO_START	(PJ_ERRNO_START_USER + PJ_ERRNO_SPACE_SIZE*2)
 *  - PJLIB_UTIL_ERRNO_START	(PJ_ERRNO_START_USER + PJ_ERRNO_SPACE_SIZE*3)
 *  - PJNATH_ERRNO_START	(PJ_ERRNO_START_USER + PJ_ERRNO_SPACE_SIZE*4)
 *  - PJMEDIA_AUDIODEV_ERRNO_START (PJ_ERRNO_START_USER + PJ_ERRNO_SPACE_SIZE*5)
 *  - PJ_SSL_ERRNO_START	   (PJ_ERRNO_START_USER + PJ_ERRNO_SPACE_SIZE*6)
 *  - PJMEDIA_VIDEODEV_ERRNO_START (PJ_ERRNO_START_USER + PJ_ERRNO_SPACE_SIZE*7)
 */

/* 内部的 */
void pj_errno_clear_handlers(void);


/****** PJ_PERROR内部 *******/

/**
 * @def pj_perror_wrapper_1(arg)
 * 用于写入详细度为 1 的日志的内部函数。如果 PJ_LOG_MAX_LEVEL 低于1，则将计算为空表达式
 * @param arg      	日志表达式
 */
#if PJ_LOG_MAX_LEVEL >= 1
    #define pj_perror_wrapper_1(arg)	pj_perror_1 arg
    /** 内部函数 */
    PJ_DECL(void) pj_perror_1(const char *sender, pj_status_t status, 
			      const char *title_fmt, ...);
#else
    #define pj_perror_wrapper_1(arg)
#endif

/**
 * @def pj_perror_wrapper_2(arg)
 * 内部函数去写日志
 * 用于写入详细度为 2的日志的内部函数。如果 PJ_LOG_MAX_LEVEL低于2，则将计算为空表达式
 * @param arg       日志表达式
 */
#if PJ_LOG_MAX_LEVEL >= 2
    #define pj_perror_wrapper_2(arg)	pj_perror_2 arg
    /** 内部函数 */
    PJ_DECL(void) pj_perror_2(const char *sender, pj_status_t status, 
			      const char *title_fmt, ...);
#else
    #define pj_perror_wrapper_2(arg)
#endif

/**
 * @def pj_perror_wrapper_3(arg)
 * 内部函数写入详细日志 3。如果 PJ_LOG_MAX_LEVEL 低于3，则将计算为空表达式
 * @param arg       日志表达式
 */
#if PJ_LOG_MAX_LEVEL >= 3
    #define pj_perror_wrapper_3(arg)	pj_perror_3 arg
    /** 内部函数 */
    PJ_DECL(void) pj_perror_3(const char *sender, pj_status_t status, 
			      const char *title_fmt, ...);
#else
    #define pj_perror_wrapper_3(arg)
#endif

/**
 * @def pj_perror_wrapper_4(arg)
 * 用详细4写日志的内部函数。如果PJ_LOG_MAX_LEVEL 低于4，则将计算为空表达式
 * @param arg       日志表达式
 */
#if PJ_LOG_MAX_LEVEL >= 4
    #define pj_perror_wrapper_4(arg)	pj_perror_4 arg
    /** 内部函数 */
    PJ_DECL(void) pj_perror_4(const char *sender, pj_status_t status, 
			      const char *title_fmt, ...);
#else
    #define pj_perror_wrapper_4(arg)
#endif

/**
 * @def pj_perror_wrapper_5(arg)
 * 用详细 5编写日志的内部函数。如果PJ_LOG_MAX_LEVEL 低于5，则将计算为空表达式
 * @param arg       日志表达式
 */
#if PJ_LOG_MAX_LEVEL >= 5
    #define pj_perror_wrapper_5(arg)	pj_perror_5 arg
    /** 内部函数 */
    PJ_DECL(void) pj_perror_5(const char *sender, pj_status_t status, 
			      const char *title_fmt, ...);
#else
    #define pj_perror_wrapper_5(arg)
#endif

/**
 * @def pj_perror_wrapper_6(arg)
 * 用于写入详细日志的内部函数 6。如果 PJ_LOG_MAX_LEVEL 低于6，则将计算为空表达式
 * @param arg       日志表达式
 */
#if PJ_LOG_MAX_LEVEL >= 6
    #define pj_perror_wrapper_6(arg)	pj_perror_6 arg
    /** Internal function. */
    PJ_DECL(void) pj_perror_6(const char *sender, pj_status_t status, 
			      const char *title_fmt, ...);
#else
    #define pj_perror_wrapper_6(arg)
#endif




PJ_END_DECL

#endif	/* __PJ_ERRNO_H__ */

