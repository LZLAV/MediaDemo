/**
 * 已完成
 *      日志工具类
 */
#ifndef __PJ_LOG_H__
#define __PJ_LOG_H__

/**
 * @file log.h
 * @brief 日志工具
 */

#include <pj/types.h>
#include <stdarg.h>

PJ_BEGIN_DECL

/**
 * @defgroup PJ_MISC 其他
 */

/**
 * @defgroup PJ_LOG 日志工具
 * @ingroup PJ_MISC
 * @{
 *
 * PJLIB 日志记录工具是一种可配置的、灵活的、方便的写入日志记录或跟踪信息的方法
 *
 * 要写入日志，可以使用如下构造：
 *
 * <pre>
 *   ...
 *   PJ_LOG(3, ("main.c", "Starting hello..."));
 *   ...
 *   PJ_LOG(3, ("main.c", "Hello world from process %d", pj_getpid()));
 *   ...
 * </pre>
 *
 * 在上面的例子中，数字 3 控制信息的详细程度（按照惯例，这意味着“信息”）。字符串 “main.c” 指定消息的源或发送方
 *
 *
 * \section pj_log_quick_sample_sec 例子
 *
 * For examples, see:
 *  - @ref page_pjlib_samples_log_c.
 *
 */

/**
 * 日志装饰标志，用 pj_log_set_decor()指定
 */
enum pj_log_decoration
{
    PJ_LOG_HAS_DAY_NAME   =    1, /**< 日期，默认没有	      */
    PJ_LOG_HAS_YEAR       =    2, /**< 年，默认没有	      */
    PJ_LOG_HAS_MONTH	  =    4, /**< 月，默认没有		      */
    PJ_LOG_HAS_DAY_OF_MON =    8, /**< 月中天数，默认没有      */
    PJ_LOG_HAS_TIME	  =   16, /**< 时间，默认有	      */
    PJ_LOG_HAS_MICRO_SEC  =   32, /**< 毫秒，默认有           */
    PJ_LOG_HAS_SENDER	  =   64, /**< 发送者，默认有      */
    PJ_LOG_HAS_NEWLINE	  =  128, /**< 用换行符终止，默认有 */
    PJ_LOG_HAS_CR	  =  256, /**< 包括回车，默认没有     */
    PJ_LOG_HAS_SPACE	  =  512, /**< 日志前包含两个空格，默认有    */
    PJ_LOG_HAS_COLOR	  = 1024, /**< 日志颜色，Win32有      */
    PJ_LOG_HAS_LEVEL_TEXT = 2048, /**< 包含级别文本字符串，默认没有	      */
    PJ_LOG_HAS_THREAD_ID  = 4096, /**< 包含线程id，默认没有   */
    PJ_LOG_HAS_THREAD_SWC = 8192, /**< 线程切换时添加标记，默认有 */
    PJ_LOG_HAS_INDENT     =16384  /**< 缩进，默认有        */
};

/**
 * 写日志信息
 * 这是用于将文本写入日志后端的主要宏定义
 *
 * @param level	    日志详细级别。数字越小表示重要性越高，级别 0表示致命错误。只允许数字参数（例如不允许变量）
 * @param arg	    'printf' 类参数，第一个参数是发送方，第二个参数是格式字符串，下面的参数是适合格式字符串的可变数量的参数
 *
 * Sample:
 * \verbatim
   PJ_LOG(2, (__FILE__, "current value is %d", value));
   \endverbatim
 * @hideinitializer
 */
#define PJ_LOG(level,arg)	do { \
				    if (level <= pj_log_get_level()) { \
					pj_log_wrapper_##level(arg); \
				    } \
				} while (0)

/**
 * 注册到日志子系统以将实际日志消息写入某个输出设备的函数的签名
 *
 * @param level	    日志级别
 * @param data	    日志消息，将以NULL结尾
 * @param len	    信息长度
 */
typedef void pj_log_func(int level, const char *data, int len);

/**
 * 前端记录器函数使用的默认日志编写器函数
 * 此函数仅将日志消息打印到标准输出。应用程序通常不需要调用此函数，而是使用PJ_LOG 宏
 *
 * @param level	    日志级别
 * @param buffer    日志消息
 * @param len	    信息长度
 */
PJ_DECL(void) pj_log_write(int level, const char *buffer, int len);


#if PJ_LOG_MAX_LEVEL >= 1

/**
 * 写日志
 *
 * @param sender    信息的源
 * @param level	    详细级别
 * @param format    格式
 * @param marker    标记
 */
PJ_DECL(void) pj_log(const char *sender, int level, 
		     const char *format, va_list marker);

/**
 * 更改日志输出功能。前端日志函数将调用此函数将实际消息写入所需设备。默认情况下，前端函数使用 pj_log_write()来编写消息，
 * 除非通过调用此函数对其进行了更改
 *
 * @param func	    将被调用以将日志消息写入所需设备的函数
 */
PJ_DECL(void) pj_log_set_log_func( pj_log_func *func );

/**
 * 获取用于写入日志消息的当前日志输出函数
 *
 * @return	    当前的日志输出函数
 */
PJ_DECL(pj_log_func*) pj_log_get_log_func(void);

/**
 * 设置最大日志级别。应用程序可以调用此函数来设置日志消息的详细程度。值越大，日志消息将打印得越详细。
 * 但是，详细性的最大级别不能超过 PJ_LOG_MAX_LEVEL 的编译时值
 *
 * @param level	    日志消息的最大详细级别（6=非常详细..1=仅错误，0=禁用）
 */
PJ_DECL(void) pj_log_set_level(int level);

/**
 * 获取当前最大日志详细级别
 *
 * @return	    当前日志最大级别
 */
#if 1
PJ_DECL(int) pj_log_get_level(void);
#else
PJ_DECL_DATA(int) pj_log_max_level;
#define pj_log_get_level()  pj_log_max_level
#endif

/**
 * 设置日志装饰。日志装饰标志控制打印到实际消息旁边的输出设备的内容。例如，应用程序可以指定在每个日志消息中显示日期/时间信息
 *
 * @param decor	    位掩码结合 pj_log_decoration 来控制日志消息的布局
 */
PJ_DECL(void) pj_log_set_decor(unsigned decor);

/**
 * 获取当前日志装饰标志
 *
 * @return	    日志装饰标识
 */
PJ_DECL(unsigned) pj_log_get_decor(void);

/**
 * 向日志消息添加缩进。Indentation 将在消息前添加 PJ_LOG_INDENT_CHAR，用于显示函数调用的深度
 *
 * @param indent    要加或减的缩进。正值加当前缩进，负值减当前缩进
 */
PJ_DECL(void) pj_log_add_indent(int indent);

/**
 * 按默认值（PJ_LOG_INDENT）向右缩进
 */
PJ_DECL(void) pj_log_push_indent(void);

/**
 * 按默认值（PJ_LOG_INDENT）弹出缩进（向左）
 */
PJ_DECL(void) pj_log_pop_indent(void);

/**
 * 设置日志消息颜色
 *
 * @param level	    将更改颜色的日志级别
 * @param color	    想要的颜色
 */
PJ_DECL(void) pj_log_set_color(int level, pj_color_t color);

/**
 * 获取日志消息的颜色
 *
 * @param level	    日志级别，返回颜色
 * @return	    日志颜色
 */
PJ_DECL(pj_color_t) pj_log_get_color(int level);

/**
 * pj_init()调用的内部函数
 */
pj_status_t pj_log_init(void);

#else	/* #if PJ_LOG_MAX_LEVEL >= 1 */

/**
 * Change log output function. The front-end logging functions will call
 * this function to write the actual message to the desired device. 
 * By default, the front-end functions use pj_log_write() to write
 * the messages, unless it's changed by calling this function.
 *
 * @param func	    The function that will be called to write the log
 *		    messages to the desired device.
 */
#  define pj_log_set_log_func(func)

/**
 * Write to log.
 *
 * @param sender    Source of the message.
 * @param level	    Verbosity level.
 * @param format    Format.
 * @param marker    Marker.
 */
#  define pj_log(sender, level, format, marker)

/**
 * Set maximum log level. Application can call this function to set 
 * the desired level of verbosity of the logging messages. The bigger the
 * value, the more verbose the logging messages will be printed. However,
 * the maximum level of verbosity can not exceed compile time value of
 * PJ_LOG_MAX_LEVEL.
 *
 * @param level	    The maximum level of verbosity of the logging
 *		    messages (6=very detailed..1=error only, 0=disabled)
 */
#  define pj_log_set_level(level)

/**
 * Set log decoration. The log decoration flag controls what are printed
 * to output device alongside the actual message. For example, application
 * can specify that date/time information should be displayed with each
 * log message.
 *
 * @param decor	    Bitmask combination of #pj_log_decoration to control
 *		    the layout of the log message.
 */
#  define pj_log_set_decor(decor)

/**
 * Add indentation to log message. Indentation will add PJ_LOG_INDENT_CHAR
 * before the message, and is useful to show the depth of function calls.
 *
 * @param indent    The indentation to add or substract. Positive value
 * 		    adds current indent, negative value subtracts current
 * 		    indent.
 */
#  define pj_log_add_indent(indent)

/**
 * Push indentation to the right by default value (PJ_LOG_INDENT).
 */
#  define pj_log_push_indent()

/**
 * Pop indentation (to the left) by default value (PJ_LOG_INDENT).
 */
#  define pj_log_pop_indent()

/**
 * Set color of log messages.
 *
 * @param level	    Log level which color will be changed.
 * @param color	    Desired color.
 */
#  define pj_log_set_color(level, color)

/**
 * Get current maximum log verbositylevel.
 *
 * @return	    Current log maximum level.
 */
#  define pj_log_get_level()	0

/**
 * Get current log decoration flag.
 *
 * @return	    Log decoration flag.
 */
#  define pj_log_get_decor()	0

/**
 * Get color of log messages.
 *
 * @param level	    Log level which color will be returned.
 * @return	    Log color.
 */
#  define pj_log_get_color(level) 0


/**
 * Internal.
 */
#   define pj_log_init()	PJ_SUCCESS

#endif	/* #if PJ_LOG_MAX_LEVEL >= 1 */

/** 
 * @}
 */

/* **************************************************************************/
/*
 * 日志函数实现原型
 * PJ_LOG 宏根据调用宏时指定的详细级别调用这些函数。应用程序通常不需要直接调用这些函数
 */

/**
 * @def pj_log_wrapper_1(arg)
 * 用于写入详细度为 1 的日志的内部函数。如果 PJ_LOG_MAX_LEVEL低于1，则将计算为空表达式
 * @param arg       日志表达
 */
#if PJ_LOG_MAX_LEVEL >= 1
    #define pj_log_wrapper_1(arg)	pj_log_1 arg
    /** 内部函数 */
    PJ_DECL(void) pj_log_1(const char *src, const char *format, ...);
#else
    #define pj_log_wrapper_1(arg)
#endif

/**
 * @def pj_log_wrapper_2(arg)
 * 用于写入详细度为 2 的日志的内部函数。如果 PJ_LOG_MAX_LEVEL 低于 2，则将计算为空表达式
 * @param arg       日志表达
 */
#if PJ_LOG_MAX_LEVEL >= 2
    #define pj_log_wrapper_2(arg)	pj_log_2 arg
    /** 内部函数 */
    PJ_DECL(void) pj_log_2(const char *src, const char *format, ...);
#else
    #define pj_log_wrapper_2(arg)
#endif

/**
 * @def pj_log_wrapper_3(arg)
 * 用于写入详细度为 3 的日志的内部函数。如果 PJ_LOG_MAX_LEVEL 低于 3，则将计算为空表达式
 * @param arg       日志表达
 */
#if PJ_LOG_MAX_LEVEL >= 3
    #define pj_log_wrapper_3(arg)	pj_log_3 arg
    /** 内部函数 */
    PJ_DECL(void) pj_log_3(const char *src, const char *format, ...);
#else
    #define pj_log_wrapper_3(arg)
#endif

/**
 * @def pj_log_wrapper_4(arg)
 * 用于写入详细度为 4 的日志的内部函数。如果 PJ_LOG_MAX_LEVEL 低于 4，则将计算为空表达式
 * @param arg       日志表达
 */
#if PJ_LOG_MAX_LEVEL >= 4
    #define pj_log_wrapper_4(arg)	pj_log_4 arg
    /** 内部函数 */
    PJ_DECL(void) pj_log_4(const char *src, const char *format, ...);
#else
    #define pj_log_wrapper_4(arg)
#endif

/**
 * @def pj_log_wrapper_5(arg)
 * 用于写入详细度为 5 的日志的内部函数。如果 PJ_LOG_MAX_LEVEL 低于 5，则将计算为空表达式
 * @param arg       日志表达
 */
#if PJ_LOG_MAX_LEVEL >= 5
    #define pj_log_wrapper_5(arg)	pj_log_5 arg
    /** 内部函数 */
    PJ_DECL(void) pj_log_5(const char *src, const char *format, ...);
#else
    #define pj_log_wrapper_5(arg)
#endif

/**
 * @def pj_log_wrapper_6(arg)
 * 用于写入详细度为 6 的日志的内部函数。如果 PJ_LOG_MAX_LEVEL 低于 6，则将计算为空表达式
 * @param arg       日志表达
 */
#if PJ_LOG_MAX_LEVEL >= 6
    #define pj_log_wrapper_6(arg)	pj_log_6 arg
    /** 内部函数 */
    PJ_DECL(void) pj_log_6(const char *src, const char *format, ...);
#else
    #define pj_log_wrapper_6(arg)
#endif


PJ_END_DECL 

#endif  /* __PJ_LOG_H__ */

