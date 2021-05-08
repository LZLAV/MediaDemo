/**
 * 已完成：
 *  包含的功能点：
 *      1. 系统信息
 *      2. 线程
 *          1. 本地存储
 *      3. 原子量
 *      4. 互斥量
 *          1. 互斥锁
 *          2. 读写互斥量
 *      5. 临界区域
 *      6. 信号量
 *      7. 事件对象
 *      8. 时间
 *      9. 高分辨率时间戳
 *      10. 应用程序主函数
 */
#ifndef __PJ_OS_H__
#define __PJ_OS_H__

/**
 * @file os.h
 * @brief 操作系统依赖的函数
 */
#include <pj/types.h>

PJ_BEGIN_DECL

/**
 * @defgroup PJ_OS 操作系统依赖的功能
 */


/* **************************************************************************/
/**
 * @defgroup PJ_SYS_INFO 系统信息
 * @ingroup PJ_OS
 * @{
 */

/**
 * 这些枚举包含表示支持其他系统功能的常量。通过 pj_sys_info structure 的 "flags" 字段
 */
typedef enum pj_sys_info_flag
{
    /**
     * 支持Apple iOS后台功能
     */
    PJ_SYS_HAS_IOS_BG = 1

} pj_sys_info_flag;


/**
 * 此结构包含有关系统的信息。使用 pj_get_sys_info() 获取系统信息
 */
typedef struct pj_sys_info
{
    /**
     * 包含处理器信息（例如"i386","x86_64"）的空终止字符串。如果无法获取值，则可能包含空字符串
     */
    pj_str_t	machine;

    /**
     * 标识系统操作的以 Null结尾的字符串 (例如"Linux","win32", "wince")。如果无法获取值，则可能包含空字符串
     */
    pj_str_t	os_name;

    /**
     * 包含操作系统版本号的数字。按照惯例，此字段分为四个字节，其中最高顺序的字节包含操作系统的最主要版本，
     * 下一个较低有效字节包含较低主要版本，依此类推。操作系统版本号如何映射到这四个字节对于每个操作系统都是特定的。
     * 例如，Linux-2.6.32-28 将产生 "os_ver" 值 0x0206201c，而对于Windows 7，它将是 0x06010000
     * （因为对于Windows 7，dwMajorVersion 是6，dwMinorVersion是1）。
     * 如果无法获取操作系统版本，则此字段可能包含零
     */
    pj_uint32_t	os_ver;

    /**
     * 以Null结尾的字符串，标识用于构建库的SDK名称（例如，"glibc", "uclibc", "msvc", "wince"）。
     * 如果无法获取值，则可能包含空字符串
     */
    pj_str_t	sdk_name;

    /**
     * 包含SDK版本的数字，使用编号约定作为 "os_ver"字段。
     * 如果无法获取版本，则该值将为零
     */
    pj_uint32_t	sdk_ver;

    /**
     * 以null结尾的较长字符串，用尽可能多的信息标识底层系统
     */
    pj_str_t	info;

    /**
     * 包含系统特定信息的其他标志。该值是 pj_sys_info_flag 常量的位掩码
     */
    pj_uint32_t	flags;

} pj_sys_info;


/**
 * 获取这个系统的信息
 *
 * @return	系统信息结构体
 */
PJ_DECL(const pj_sys_info*) pj_get_sys_info(void);

/*
 * @}
 */

/* **************************************************************************/
/**
 * @defgroup PJ_THREAD 线程
 * @ingroup PJ_OS
 * @{
 * 此模块提供了多线程API
 *
 * \section pj_thread_examples_sec 例子
 *
 * For examples, please see:
 *  - \ref page_pjlib_thread_test
 *  - \ref page_pjlib_sleep_test
 *
 */

/**
 * 线程创建标识：
 * - PJ_THREAD_SUSPENDED: 指定应创建挂起的线程
 */
typedef enum pj_thread_create_flags
{
    PJ_THREAD_SUSPENDED = 1
} pj_thread_create_flags;


/**
 * 定义线程进入函数的类型
 */
typedef int (PJ_THREAD_FUNC pj_thread_proc)(void*);

/**
 * 线程结构的大小
 */
#if !defined(PJ_THREAD_DESC_SIZE)
#   define PJ_THREAD_DESC_SIZE	    (64)
#endif

/**
 * 线程结构，当线程是由外部或本机API创建时的线程状态
 */
typedef long pj_thread_desc[PJ_THREAD_DESC_SIZE];

/**
 * 获取进程ID
 * @return 进程ID
 */
PJ_DECL(pj_uint32_t) pj_getpid(void);

/**
 * 创建一个线程
 *
 * @param pool          从中分配线程记录的内存池
 * @param thread_name   要分配给线程的可选名称
 * @param proc          线程入口函数
 * @param arg           要传递给线程入口函数的参数
 * @param stack_size	新线程的堆栈大小，或 0 或 PJ_THREAD_DEFAULT_STACK_SIZE，以便库为堆栈选择合理的大小。对于某些系统，堆栈将从池中分配，因此池必须具有适当的容量
 * @param flags         用于线程创建的标志，它是来自pj_thread_create_flags 的枚举
 * @param thread        用来保存新创建的线程的指针
 *
 * @return	        成功返回 PJ_SUCCESS，失败返回错误码
 */
PJ_DECL(pj_status_t) pj_thread_create(  pj_pool_t *pool, 
                                        const char *thread_name,
				        pj_thread_proc *proc, 
                                        void *arg,
				        pj_size_t stack_size, 
                                        unsigned flags,
					pj_thread_t **thread );

/**
 * 将由外部或本机API创建的线程注册到 PJLIB。
 * 必须在正在注册的线程的上下文中调用此函数。
 * 当通过外部函数或API调用创建线程时，必须使用 pj_thread_register() 将其“注册”到PJLIB，以便与PJLIB的框架协作。
 * 在注册期间，需要维护一些数据，并且这些数据必须在线程的生存期内保持可用
 *
 * @param thread_name   要分配给线程的可选名称
 * @param desc          线程描述符，它必须在线程的整个生存期内都可用
 * @param thread        用来保存创建的线程句柄的指针
 *
 * @return              成功返回 PJ_SUCCESS，失败返回错误码
 */
PJ_DECL(pj_status_t) pj_thread_register ( const char *thread_name,
					  pj_thread_desc desc,
					  pj_thread_t **thread);

/**
 * 检查此线程是否已注册到 PJLIB
 *
 * @return		如果已经注册返回非 0
 */
PJ_DECL(pj_bool_t) pj_thread_is_registered(void);


/**
 * 获取线程的线程优先级值
 *
 * @param thread	线程句柄
 *
 * @return		线程优先级，失败返回 -1
 */
PJ_DECL(int) pj_thread_get_prio(pj_thread_t *thread);


/**
 * 设置线程优先级。优先级值必须在优先级值范围内，可以使用 pj_thread_get_prio_min() 和 pj_thread_get_prio_max() 函数检索该范围
 *
 * @param thread	线程句柄
 * @param prio		要为线程设置的新优先级
 *
 * @return		成功返回 PJ_SUCCESS，失败返回错误码
 */
PJ_DECL(pj_status_t) pj_thread_set_prio(pj_thread_t *thread,  int prio);

/**
 * 获取此线程可用的最低优先级值
 *
 * @param thread	线程句柄
 * @return		最小线程优先级值，错误时为-1
 */
PJ_DECL(int) pj_thread_get_prio_min(pj_thread_t *thread);


/**
 * 获取此线程可用的最高优先级值
 *
 * @param thread	线程句柄
 * @return		最大线程优先级值，错误时为-1
 */
PJ_DECL(int) pj_thread_get_prio_max(pj_thread_t *thread);


/**
 * 返回本机句柄 pj_thread_t ，以便使用本机OS API 进行操作
 *
 * @param thread	PJLIB 线程描述符
 *
 * @return		本机线程句柄。例如，当后端线程使用 pthread 时，此函数将返回指向 pthread_t 的指针，在 Windows上，此函数将返回HANDLE
 */
PJ_DECL(void*) pj_thread_get_os_handle(pj_thread_t *thread);

/**
 * 获取线程名称
 *
 * @param thread    线程句柄
 *
 * @return 线程名称为以null结尾的字符串
 */
PJ_DECL(const char*) pj_thread_get_name(pj_thread_t *thread);

/**
 * 恢复挂起的线程
 *
 * @param thread    线程句柄
 *
 * @return 成功返回 0
 */
PJ_DECL(pj_status_t) pj_thread_resume(pj_thread_t *thread);

/**
 * 获取当前线程
 *
 * @return 当前线程的线程句柄
 */
PJ_DECL(pj_thread_t*) pj_thread_this(void);

/**
 *  Join 线程，并阻止调用线程，直到指定的线程退出。如果从线程本身调用它，它将立即返回失败状态。
 *  如果指定的线程已经死了，或者它不存在，函数将立即返回successful状态
 * @param thread    线程句柄
 *
 * @return 成功返回 PJ_SUCCESS
 */
PJ_DECL(pj_status_t) pj_thread_join(pj_thread_t *thread);


/**
 * 销毁线程并释放分配给该线程的资源。
 * 但是，只有在创建线程的池被销毁时，才释放为 pj_thread_t 线程本身分配的内存
 *
 * @param thread    线程句柄
 *
 * @return 成功返回 0
 */
PJ_DECL(pj_status_t) pj_thread_destroy(pj_thread_t *thread);


/**
 * 将当前线程休眠指定毫秒
 *
 * @param 延迟毫秒
 *
 * @return 成功返回 0
 */
PJ_DECL(pj_status_t) pj_thread_sleep(unsigned msec);

/**
 * @def PJ_CHECK_STACK()
 * PJ_CHECK_STACK（）宏用于检查堆栈的健全性。
 * 操作系统实现可以检查没有发生堆栈溢出，还可以收集有关堆栈使用情况的统计信息
 */
#if defined(PJ_OS_HAS_CHECK_STACK) && PJ_OS_HAS_CHECK_STACK!=0

#  define PJ_CHECK_STACK() pj_thread_check_stack(__FILE__, __LINE__)

/** @internal
 * 堆栈检查的实现。
 */
PJ_DECL(void) pj_thread_check_stack(const char *file, int line);

/** @internal
 * 获取最大堆栈使用率统计信息。
 */
PJ_DECL(pj_uint32_t) pj_thread_get_stack_max_usage(pj_thread_t *thread);

/** @internal
 * 转储线程堆栈状态
 */
PJ_DECL(pj_status_t) pj_thread_get_stack_info(pj_thread_t *thread,
					      const char **file,
					      int *line);
#else

#  define PJ_CHECK_STACK()
/** pj_thread_get_stack_max_usage() for the thread */
#  define pj_thread_get_stack_max_usage(thread)	    0
/** pj_thread_get_stack_info() for the thread */
#  define pj_thread_get_stack_info(thread,f,l)	    (*(f)="",*(l)=0)
#endif	/* PJ_OS_HAS_CHECK_STACK */

/**
 * @}
 */

/* **************************************************************************/
/**
 * @defgroup PJ_SYMBIAN_OS 特定于Symbian操作系统
 * @ingroup PJ_OS
 * @{
 * Symbian操作系统特有的功能
 *
 * Symbian操作系统强烈反对使用轮询，因为这会浪费CPU资源，而是提供活动对象和活动调度程序模式，允许应用程序（在本例中是 PJLIB）注册异步任务。Symbian的 PJLIB 端口符合此建议行为。
 * 因此，Symbian 的 PJLIB几乎没有什么变化：
 * 	-计时器堆（请参阅 @ref PJ_TIMER）是用活动对象框架实现的，注册到计时器堆的每个计时器条目都将向活动计划程序注册一个活动对象。因此，不再需要使用 pj_timer_heap_poll()轮询计时器堆，
 * 	此函数的计算结果将为 nothing
 * 	-ioqueue（请参阅@ref PJ_IOQUEUE）也是用活动对象框架实现的，每个异步操作都将向活动调度器注册一个活动对象。因此，不再需要使用 pj_ioqueue_poll() 轮询ioqueue，
 * 	此函数的计算结果将为nothing。
 *
 * 由于不再需要计时器堆和 ioqueue 轮询，Symbian应用程序现在可以通过调用User::WaitForAnyRequest() 和 CActiveScheduler::RunIfReady()来轮询所有事件。
 * PJLIB提供了一个调用这两个函数的瘦包装器，称为pj_symbianos_poll()
 */
 
/**
 *
 * 等待任何 Symbian活动对象的完成。当未指定超时值（ms_timeout参数为-1）时，此函数是一个简易包装器，它调用 User::WaitForAnyRequest()
 * 和CActiveScheduler::RunIfReady() 。如果指定了超时值，此函数将计时器项安排到计时器堆（该计时器堆是活动对象），以限制事件发生的等待时间。
 * 调度计时器条目是一项昂贵的操作，因此应用程序应该只在真正需要时指定超时值（例如，当不确定应用程序中当前运行的其他活动对象时）
 *
 * @param priority	要轮询的活动对象的最小优先级，这些值来自 CActive::TPriority常量。如果给定-1，则将使用CActive::EPriorityStandard
 * @param ms_timeout	等待的可选超时。应用程序应该指定-1，让函数无限期地等待任何事件
 *
 * @return		如果在轮询期间执行了任何事件，则为 PJ_TRUE。如果指定了ms_timeout参数（即值不是-1），并且超时计时器结束时没有执行任何事件，则此函数只返回PJ_FALSE
 */
PJ_DECL(pj_bool_t) pj_symbianos_poll(int priority, int ms_timeout);


/**
 * 此结构声明了在调用 pj_symbianos_set_params() 时可以指定的Symbian OS特定参数
 */
typedef struct pj_symbianos_params 
{
    /**
     * PJLIB使用的可选 RSocketServ 实例。如果该值为NULL，则在调用 pj_init() 时，PJLIB将创建一个新的 RSocketServ实例
     */
    void	*rsocketserv;
    
    /**
     * PJLIB在创建套接字时使用的可选 RConnection实例。如果此值为NULL，则在创建套接字时不会指定RConnection
     */
    void	*rconnection;
    
    /**
     * PJLIB使用的可选 RHostResolver 实例。如果此值为空，则在调用pj_Uinit（）时将创建一个新的RHostResolver实例
     */
    void 	*rhostresolver;
     
    /**
     * PJLIB使用的IPv6实例的可选 RHostResolver
     * 如果此值为NULL，则在调用 pj_init() 时将创建一个新的 RHostResolver实例
     */
    void 	*rhostresolver6;
     
} pj_symbianos_params;

/**
 * 指定 PJLIB要使用的Symbian OS参数。必须在调用 pj_init() 之前调用此函数
 *
 * @param prm		Symbian特定参数
 *
 * @return		参数设置成功则返回 PJ_SUCCESS
 */
PJ_DECL(pj_status_t) pj_symbianos_set_params(pj_symbianos_params *prm);

/**
 *  通知 PJLIB 接入点连接已关闭或不可用，PJLIB不应尝试访问 Symbian socket API（尤其是发送数据包的套接字API）。
 *  当 RConnection重新连接到不同的访问点时发送数据包可能会导致函数的 WaitForRequest() 无限期阻塞
 *  
 *  @param up		如果设置为PJ_FALSE，它将导致PJLIB不尝试访问套接字API，并且会立即返回错误
 */
PJ_DECL(void) pj_symbianos_set_connection_status(pj_bool_t up);

/**
 * @}
 */
 
/* **************************************************************************/
/**
 * @defgroup PJ_TLS 线程本地存储
 * @ingroup PJ_OS
 * @{
 */

/** 
 * 分配线程本地存储索引。索引处变量的初始值为零
 *
 * @param index	    保存返回值的指针
 * @return	    成功返回 PJ_SUCCESS，失败返回错误码
 */
PJ_DECL(pj_status_t) pj_thread_local_alloc(long *index);

/**
 * 释放线程局部变量
 *
 * @param index	    变量索引
 */
PJ_DECL(void) pj_thread_local_free(long index);

/**
 * 设置线程局部变量的值
 *
 * @param index	    变量的索引
 * @param value	    值
 */
PJ_DECL(pj_status_t) pj_thread_local_set(long index, void *value);

/**
 * 获取线程局部变量的值
 *
 * @param index	    变量的索引
 * @return	    值
 */
PJ_DECL(void*) pj_thread_local_get(long index);


/**
 * @}
 */


/* **************************************************************************/
/**
 * @defgroup PJ_ATOMIC 原子变量
 * @ingroup PJ_OS
 * @{
 *
 * 这个模块提供了操作原子变量的API
 *
 * \section pj_atomic_examples_sec Examples
 *
 * For some example codes, please see:
 *  - @ref page_pjlib_atomic_test
 */


/**
 * 创建原子变量
 *
 * @param pool	    池
 * @param initial   原子变量的初始值。
 * @param atomic    返回时保存原子变量的指针
 *
 * @return	    成功返回 PJ_SUCCESS，失败返回错误码
 */
PJ_DECL(pj_status_t) pj_atomic_create( pj_pool_t *pool, 
				       pj_atomic_value_t initial,
				       pj_atomic_t **atomic );

/**
 * 销毁原子变量
 *
 * @param atomic_var	原子变量
 *
 * @return 成功返回 PJ_SUCCESS
 */
PJ_DECL(pj_status_t) pj_atomic_destroy( pj_atomic_t *atomic_var );

/**
 * 设置原子类型的值，并返回上一个值
 *
 * @param atomic_var	原子变量
 * @param value		要设置为变量的值
 */
PJ_DECL(void) pj_atomic_set( pj_atomic_t *atomic_var, 
			     pj_atomic_value_t value);

/**
 * 获取原子类型的值
 *
 * @param atomic_var	原子变量
 *
 * @return 原子变量的值
 */
PJ_DECL(pj_atomic_value_t) pj_atomic_get(pj_atomic_t *atomic_var);

/**
 * 增加原子类型的值
 *
 * @param atomic_var	原子变量
 */
PJ_DECL(void) pj_atomic_inc(pj_atomic_t *atomic_var);

/**
 * 增加原子类型的值并得到结果
 *
 * @param atomic_var	原子变量
 *
 * @return              递增的值
 */
PJ_DECL(pj_atomic_value_t) pj_atomic_inc_and_get(pj_atomic_t *atomic_var);

/**
 * 减少原子类型的值
 *
 * @param atomic_var	原子变量
 */
PJ_DECL(void) pj_atomic_dec(pj_atomic_t *atomic_var);

/**
 * 减少原子类型的值并得到结果
 *
 * @param atomic_var	原子变量
 *
 * @return              减少的值
 */
PJ_DECL(pj_atomic_value_t) pj_atomic_dec_and_get(pj_atomic_t *atomic_var);

/**
 * 向原子类型添加值
 *
 * @param atomic_var	原子变量
 * @param value		增值
 */
PJ_DECL(void) pj_atomic_add( pj_atomic_t *atomic_var,
			     pj_atomic_value_t value);

/**
 * 将值添加到原子类型并获得结果
 *
 * @param atomic_var	原子变量
 * @param value		增值
 *
 * @return              增加后的值
 */
PJ_DECL(pj_atomic_value_t) pj_atomic_add_and_get( pj_atomic_t *atomic_var,
			                          pj_atomic_value_t value);

/**
 * @}
 */

/* **************************************************************************/
/**
 * @defgroup PJ_MUTEX 互斥量
 * @ingroup PJ_OS
 * @{
 *
 * 互斥操作。或者，应用程序可以对锁对象使用更高的抽象，这为各种锁机制（包括互斥锁）提供了统一的API。更多信息请参见
 * @ref PJ_LOCK
 */

/**
 * 互斥量类型：
 *  - PJ_MUTEX_DEFAULT: 默认互斥类型，与系统相关
 *  - PJ_MUTEX_SIMPLE: 非递归互斥
 *  - PJ_MUTEX_RECURSE: 递归互斥
 */
typedef enum pj_mutex_type_e
{
    PJ_MUTEX_DEFAULT,
    PJ_MUTEX_SIMPLE,
    PJ_MUTEX_RECURSE
} pj_mutex_type_e;


/**
 * 创建指定类型的互斥量
 *
 * @param pool	    池
 * @param name	    要与互斥量关联的名称（用于调试）
 * @param type	    互斥量的类型，类型为 pj_mutex_type_e
 * @param mutex	    指针，以保存返回的互斥量实例
 *
 * @return	    成功返回 PJ_SUCCESS，否则返回错误码
 */
PJ_DECL(pj_status_t) pj_mutex_create(pj_pool_t *pool, 
                                     const char *name,
				     int type, 
                                     pj_mutex_t **mutex);

/**
 * 创建简单的非递归互斥
 * 此函数是 pj_mutex_create 的简单包装，用于创建非递归互斥量
 *
 * @param pool	    池
 * @param name	    互斥量名称
 * @param mutex	    保存返回的互斥实例的指针
 *
 * @return	    成功返回PJ_SUCCESS，否则返回错误码
 */
PJ_DECL(pj_status_t) pj_mutex_create_simple( pj_pool_t *pool, const char *name,
					     pj_mutex_t **mutex );

/**
 * 创建递归互斥。
 * 此函数是 pj_mutex_create 的简单包装器，用于创建递归 mutex
 *
 * @param pool	    池
 * @param name	    互斥量名称
 * @param mutex	    保存返回的互斥实例的指针
 *
 * @return	    成功返回 PJ_SUCCESS，否则返回错误码
 */
PJ_DECL(pj_status_t) pj_mutex_create_recursive( pj_pool_t *pool,
					        const char *name,
						pj_mutex_t **mutex );

/**
 * 获取互斥锁。
 *
 * @param mutex	    互斥量
 * @return	    成功返回 PJ_SUCCESS，否则返回错误码
 */
PJ_DECL(pj_status_t) pj_mutex_lock(pj_mutex_t *mutex);

/**
 * 释放互斥锁
 *
 * @param mutex	    互斥量
 * @return	    成功返回PJ_SUCCESS，否则返回错误码
 */
PJ_DECL(pj_status_t) pj_mutex_unlock(pj_mutex_t *mutex);

/**
 * 尝试获取互斥锁
 *
 * @param mutex	    互斥量
 * @return	    成功返回 PJ_SUCCESS，锁不能被获取则返回错误码
 */
PJ_DECL(pj_status_t) pj_mutex_trylock(pj_mutex_t *mutex);

/**
 * 销毁互斥量
 *
 * @param mutex	    互斥量
 * @return	    成功返回PJ_SUCCESS，否则返回错误码
 */
PJ_DECL(pj_status_t) pj_mutex_destroy(pj_mutex_t *mutex);

/**
 * 确定调用线程是否拥有互斥锁（仅在设置了 PJ_DEBUG 时可用）。
 * @param mutex	    互斥量
 * @return	    拥有返回非零值
 */
PJ_DECL(pj_bool_t) pj_mutex_is_locked(pj_mutex_t *mutex);

/**
 * @}
 */

/* **************************************************************************/
/**
 * @defgroup PJ_RW_MUTEX 读写互斥量
 * @ingroup PJ_OS
 * @{
 * 读写互斥量是一个经典的同步对象，其中多个读写器可以获取互斥，但只有一个读写器可以获取互斥
 */

/**
 * 读写互斥量的不透明声明
 * 读写互斥量是一个经典的同步对象，其中多个读写器可以获取互斥，但只有一个读写器可以获取互斥
 */
typedef struct pj_rwmutex_t pj_rwmutex_t;

/**
 * 常见读写互斥量
 *
 * @param pool	    互斥量分配内存的内存池
 * @param name	    互斥量名称
 * @param mutex	    新创建互斥量的指针
 *
 * @return	    成功返回PJ_SUCCESS，否则返回错误码
 */
PJ_DECL(pj_status_t) pj_rwmutex_create(pj_pool_t *pool, const char *name,
				       pj_rwmutex_t **mutex);

/**
 * 锁定互斥量以进行读取
 *
 * @param mutex	    互斥量
 * @return	    成功返回 PJ_SUCCESS，否则返回错误码
 */
PJ_DECL(pj_status_t) pj_rwmutex_lock_read(pj_rwmutex_t *mutex);

/**
 * 锁定互斥量以进行写入
 *
 * @param mutex	    互斥量
 * @return	    成功返回 PJ_SUCCESS，否则返回错误码
 */
PJ_DECL(pj_status_t) pj_rwmutex_lock_write(pj_rwmutex_t *mutex);

/**
 * 释放读取锁
 *
 * @param mutex	    互斥量
 * @return	    成功返回 PJ_SUCCESS，否则返回错误码
 */
PJ_DECL(pj_status_t) pj_rwmutex_unlock_read(pj_rwmutex_t *mutex);

/**
 * 释放写入锁
 *
 * @param mutex	    互斥量
 * @return	    成功返回 PJ_SUCCESS，否则返回错误码
 */
PJ_DECL(pj_status_t) pj_rwmutex_unlock_write(pj_rwmutex_t *mutex);

/**
 * 销毁读写互斥量
 *
 * @param mutex	    互斥量
 * @return	    成功返回 PJ_SUCCESS，否则返回错误码
 */
PJ_DECL(pj_status_t) pj_rwmutex_destroy(pj_rwmutex_t *mutex);


/**
 * @}
 */


/* **************************************************************************/
/**
 * @defgroup PJ_CRIT_SEC 临界区域
 * @ingroup PJ_OS
 * @{
 * 临界截面保护可用于保护以下区域：
 *  -需要互斥保护
 *  -创建互斥锁太贵了
 *  -在该地区的时间非常短暂
 *
 * Critical section是一个全局对象，它阻止任何线程进入受 Critical section保护的任何区域，只要线程已经在该区域中
 *
 * Critial section 不是递归
 *
 * 应用程序在持有 critical section 时，不能调用任何可能导致当前线程阻塞的函数（如分配内存、执行I/O、锁定互斥等）
 */
/**
 * 进入临界区域
 */
PJ_DECL(void) pj_enter_critical_section(void);

/**
 * 离开临界区域
 */
PJ_DECL(void) pj_leave_critical_section(void);

/**
 * @}
 */

/* **************************************************************************/
#if defined(PJ_HAS_SEMAPHORE) && PJ_HAS_SEMAPHORE != 0
/**
 * @defgroup PJ_SEM 信号量
 * @ingroup PJ_OS
 * @{
 *
 * 此模块提供信号量的抽象（任何地方都可获得）
 */

/**
 * 创建信号量
 *
 * @param pool	    池
 * @param name	    要分配给信号量的名称（用于日志记录）
 * @param initial   信号量的初始计数
 * @param max	    信号量的最大计数
 * @param sem	    用来保存创建的信号量的指针
 *
 * @return	    成功返回 PJ_SUCCESS，否则返回错误码
 */
PJ_DECL(pj_status_t) pj_sem_create( pj_pool_t *pool, 
                                    const char *name,
				    unsigned initial, 
                                    unsigned max,
				    pj_sem_t **sem);

/**
 * 等待信号量
 *
 * @param sem	信号量
 *
 * @return	成功返回 PJ_SUCCESS，否则返回错误码
 */
PJ_DECL(pj_status_t) pj_sem_wait(pj_sem_t *sem);

/**
 * 尝试等待信号量
 *
 * @param sem	信号量
 *
 * @return	成功返回 PJ_SUCCESS，否则返回错误码
 */
PJ_DECL(pj_status_t) pj_sem_trywait(pj_sem_t *sem);

/**
 * 释放信号量
 *
 * @param sem	信号量
 *
 * @return	成功返回 PJ_SUCCESS，否则返回错误码
 */
PJ_DECL(pj_status_t) pj_sem_post(pj_sem_t *sem);

/**
 * 销毁信号量
 *
 * @param sem	信号量
 *
 * @return	成功返回 PJ_SUCCESS，否则返回错误码
 */
PJ_DECL(pj_status_t) pj_sem_destroy(pj_sem_t *sem);

/**
 * @}
 */
#endif	/* PJ_HAS_SEMAPHORE */


/* **************************************************************************/
#if defined(PJ_HAS_EVENT_OBJ) && PJ_HAS_EVENT_OBJ != 0
/**
 * @defgroup PJ_EVENT 事件对象
 * @ingroup PJ_OS
 * @{
 *
 * 此模块在可用的情况下提供对事件对象（例如Win32事件）的抽象。事件对象可用于线程间的同步
 */

/**
 * 创建事件对象
 *
 * @param pool		池
 * @param name		事件对象的名称（用于日志记录）
 * @param manual_reset	指定事件是否为手动重置
 * @param initial	指定事件对象的初始状态
 * @param event		保存返回的事件对象的指针
 *
 * @return 事件句柄，失败返回 NULL
 */
PJ_DECL(pj_status_t) pj_event_create(pj_pool_t *pool, const char *name,
				     pj_bool_t manual_reset, pj_bool_t initial,
				     pj_event_t **event);

/**
 * 等待事件发出信号
 *
 * @param event	    事件对象
 *
 * @return 成功返回 0
 */
PJ_DECL(pj_status_t) pj_event_wait(pj_event_t *event);

/**
 * 尝试等待事件对象发出信号
 *
 * @param event 事件对象
 *
 * @return 成功返回 0
 */
PJ_DECL(pj_status_t) pj_event_trywait(pj_event_t *event);

/**
 * 将事件对象状态设置为 signaled。对于自动重置事件，这将只释放等待事件的第一个线程。对于手动重置事件，状态保持为信号状态，直到事件重置。
 * 如果没有线程在等待事件，则事件对象状态保持有信号
 *
 * @param event	    事件对象
 *
 * @return 成功返回 0
 */
PJ_DECL(pj_status_t) pj_event_set(pj_event_t *event);

/**
 * 将事件对象设置为 signaled状态以释放适当数量的等待线程，然后将事件对象重置为 non-signaled。对于手动重置事件，此函数将释放所有等待的线程。
 * 对于自动重置事件，此函数只释放一个等待线程
 *
 * @param event	    事件对象
 *
 * @return 成功返回 0
 */
PJ_DECL(pj_status_t) pj_event_pulse(pj_event_t *event);

/**
 * 将事件对象状态设置为non-signaled
 *
 * @param event	    事件对象
 *
 * @return 成功返回 0
 */
PJ_DECL(pj_status_t) pj_event_reset(pj_event_t *event);

/**
 * 销毁事件对象
 *
 * @param event	    事件对象
 *
 * @return 成功返回 0
 */
PJ_DECL(pj_status_t) pj_event_destroy(pj_event_t *event);

/**
 * @}
 */
#endif	/* PJ_HAS_EVENT_OBJ */

/* **************************************************************************/
/**
 * @addtogroup PJ_TIME 时间数据类型和操作
 * @ingroup PJ_OS
 * @{
 * 此模块提供用于操作时间的API
 *
 * \section pj_time_examples_sec Examples
 *
 * For examples, please see:
 *  - \ref page_pjlib_sleep_test
 */

/**
 * 获取本地表示中的当前时间
 *
 * @param tv	变量来存储结果
 *
 * @return 成功返回 0
 */
PJ_DECL(pj_status_t) pj_gettimeofday(pj_time_val *tv);


/**
 * 将时间值解析为日期/时间表示形式
 *
 * @param tv	时间
 * @param pt	变量来存储日期时间结果
 *
 * @return 成功返回 0
 */
PJ_DECL(pj_status_t) pj_time_decode(const pj_time_val *tv, pj_parsed_time *pt);

/**
 * 对日期/时间值进行编码
 *
 * @param pt	日期/时间
 * @param tv	用于存储时间值结果的变量
 *
 * @return 成功返回 0
 */
PJ_DECL(pj_status_t) pj_time_encode(const pj_parsed_time *pt, pj_time_val *tv);

/**
 * 将当地时间转换为GMT
 *
 * @param tv	转换的时间
 *
 * @return 成功返回 0
 */
PJ_DECL(pj_status_t) pj_time_local_to_gmt(pj_time_val *tv);

/**
 * 将GMT转换为当地时间
 *
 * @param tv	转换的时间
 *
 * @return 成功返回 0
 */
PJ_DECL(pj_status_t) pj_time_gmt_to_local(pj_time_val *tv);

/**
 * @}
 */

/* **************************************************************************/
#if defined(PJ_TERM_HAS_COLOR) && PJ_TERM_HAS_COLOR != 0

/**
 * @defgroup PJ_TERM Terminal
 * @ingroup PJ_OS
 * @{
 */

/**
 * 设置当前终端的颜色
 *
 * @param color	    RGB 颜色
 *
 * @return 成功返回 0
 */
PJ_DECL(pj_status_t) pj_term_set_color(pj_color_t color);

/**
 * 获取当前终端前景色
 *
 * @return RGB color.
 */
PJ_DECL(pj_color_t) pj_term_get_color(void);

/**
 * @}
 */

#endif	/* PJ_TERM_HAS_COLOR */

/* **************************************************************************/
/**
 * @defgroup PJ_TIMESTAMP 高分辨率时间戳
 * @ingroup PJ_OS
 * @{
 *
 * PJLIB提供 高分辨率时间戳 API来访问平台提供的最高分辨率时间戳值。API可用于测量精确的运行时间，并可用于分析等应用程序。
 *
 * 时间戳值以周期表示，并且可以使用所提供的各种函数与正常时间（以秒或亚秒为单位）相关
 *
 * \section pj_timestamp_examples_sec Examples
 *
 * For examples, please see:
 *  - \ref page_pjlib_sleep_test
 *  - \ref page_pjlib_timestamp_test
 */

/*
 * 高分辨率时间戳
 */
#if defined(PJ_HAS_HIGH_RES_TIMER) && PJ_HAS_HIGH_RES_TIMER != 0

/**
 * 从某个不确定的起点得到单调的时间
 *
 * @param tv	变量来存储结果
 *
 * @return 成功返回 PJ_SUCCESS
 */
PJ_DECL(pj_status_t) pj_gettickcount(pj_time_val *tv);

/**
 * 获取高分辨率定时器值。时间值以周期形式存储
 *
 * @param ts	    高分辨率时间戳值
 * @return	    成功返回 PJ_SUCCESS，否则返回相应的错误码
 *
 * @see pj_get_timestamp_freq().
 */
PJ_DECL(pj_status_t) pj_get_timestamp(pj_timestamp *ts);

/**
 * 获取高分辨率定时器频率，以每秒周期为单位
 *
 * @param freq	    定时器频率，以每秒周期数为单位
 * @return	   成功返回 PJ_SUCCESS，否则返回相应错误码
 */
PJ_DECL(pj_status_t) pj_get_timestamp_freq(pj_timestamp *freq);

/**
 * 从32位值设置时间戳
 * @param t	    设置的时间戳
 * @param hi	    高32位部分
 * @param lo	    低32位部分
 */
PJ_INLINE(void) pj_set_timestamp32(pj_timestamp *t, pj_uint32_t hi,
				   pj_uint32_t lo)
{
    t->u32.hi = hi;
    t->u32.lo = lo;
}


/**
 * 比较时间戳 t1 和 t2
 * @param t1	    t1.
 * @param t2	    t2.
 * @return	    -1 if (t1 < t2), 1 if (t1 > t2), or 0 if (t1 == t2)
 */
PJ_INLINE(int) pj_cmp_timestamp(const pj_timestamp *t1, const pj_timestamp *t2)
{
#if PJ_HAS_INT64
    if (t1->u64 < t2->u64)
	return -1;
    else if (t1->u64 > t2->u64)
	return 1;
    else
	return 0;
#else
    if (t1->u32.hi < t2->u32.hi ||
	(t1->u32.hi == t2->u32.hi && t1->u32.lo < t2->u32.lo))
	return -1;
    else if (t1->u32.hi > t2->u32.hi ||
	     (t1->u32.hi == t2->u32.hi && t1->u32.lo > t2->u32.lo))
	return 1;
    else
	return 0;
#endif
}


/**
 * 加时间戳 t2 到 t1
 * @param t1	    t1.
 * @param t2	    t2.
 */
PJ_INLINE(void) pj_add_timestamp(pj_timestamp *t1, const pj_timestamp *t2)
{
#if PJ_HAS_INT64
    t1->u64 += t2->u64;
#else
    pj_uint32_t old = t1->u32.lo;
    t1->u32.hi += t2->u32.hi;
    t1->u32.lo += t2->u32.lo;
    if (t1->u32.lo < old)
	++t1->u32.hi;
#endif
}

/**
 * 加时间戳 t2 到 t1
 * @param t1	    t1.
 * @param t2	    t2.
 */
PJ_INLINE(void) pj_add_timestamp32(pj_timestamp *t1, pj_uint32_t t2)
{
#if PJ_HAS_INT64
    t1->u64 += t2;
#else
    pj_uint32_t old = t1->u32.lo;
    t1->u32.lo += t2;
    if (t1->u32.lo < old)
	++t1->u32.hi;
#endif
}

/**
 * 从t1中减去时间戳t2
 * @param t1	    t1.
 * @param t2	    t2.
 */
PJ_INLINE(void) pj_sub_timestamp(pj_timestamp *t1, const pj_timestamp *t2)
{
#if PJ_HAS_INT64
    t1->u64 -= t2->u64;
#else
    t1->u32.hi -= t2->u32.hi;
    if (t1->u32.lo >= t2->u32.lo)
	t1->u32.lo -= t2->u32.lo;
    else {
	t1->u32.lo -= t2->u32.lo;
	--t1->u32.hi;
    }
#endif
}

/**
 * 从t1中减去时间戳t2
 * @param t1	    t1.
 * @param t2	    t2.
 */
PJ_INLINE(void) pj_sub_timestamp32(pj_timestamp *t1, pj_uint32_t t2)
{
#if PJ_HAS_INT64
    t1->u64 -= t2;
#else
    if (t1->u32.lo >= t2)
	t1->u32.lo -= t2;
    else {
	t1->u32.lo -= t2;
	--t1->u32.hi;
    }
#endif
}

/**
 * 获取t2和t1之间的时间戳差（即t2减去t1），并返回一个32位有符号整数差
 */
PJ_INLINE(pj_int32_t) pj_timestamp_diff32(const pj_timestamp *t1,
					  const pj_timestamp *t2)
{
    /* Be careful with the signess (I think!) */
#if PJ_HAS_INT64
    pj_int64_t diff = t2->u64 - t1->u64;
    return (pj_int32_t) diff;
#else
    pj_int32 diff = t2->u32.lo - t1->u32.lo;
    return diff;
#endif
}


/**
 * 计算经过的时间，并将其存储在pj_time_val中。此函数使用当前平台可用的最高精度计算来计算运行时间，并考虑是否有浮点或64位精度算法可用。
 * 为了获得最大的可移植性，应用程序应该更喜欢使用这个函数，而不是自己计算运行时间
 *
 * @param start     开始时间戳
 * @param stop      结束时间戳
 *
 * @return	    已用时间为pj_time_val
 *
 * @see pj_elapsed_usec(), pj_elapsed_cycle(), pj_elapsed_nanosec()
 */
PJ_DECL(pj_time_val) pj_elapsed_time( const pj_timestamp *start,
                                      const pj_timestamp *stop );

/**
 * 将经过的时间计算为32位毫秒。
 * 此函数使用当前平台可用的最高精度计算来计算运行时间，并考虑是否有浮点或64位精度算法可用。
 * 为了获得最大的可移植性，应用程序应该更喜欢使用这个函数，而不是自己计算运行时间。
 *
 * @param start     开始时间戳
 * @param stop      结束时间戳
 *
 * @return	    已用时间（ms）
 *
 * @see pj_elapsed_time(), pj_elapsed_cycle(), pj_elapsed_nanosec()
 */
PJ_DECL(pj_uint32_t) pj_elapsed_msec( const pj_timestamp *start,
                                      const pj_timestamp *stop );

/**
 *  pj_elapsed_msec() 的变量，返回64位值
 */
PJ_DECL(pj_uint64_t) pj_elapsed_msec64(const pj_timestamp *start,
                                       const pj_timestamp *stop );

/**
 * 以32位微秒为单位计算经过的时间
 * 此函数使用当前平台可用的最高精度计算来计算运行时间，并考虑是否有浮点或64位精度算法可用。
 * 为了获得最大的可移植性，应用程序应该更喜欢使用这个函数，而不是自己计算运行时间
 *
 * @param start     开始时间戳
 * @param stop      结束时间戳
 *
 * @return	    已用时间（微秒）
 *
 * @see pj_elapsed_time(), pj_elapsed_cycle(), pj_elapsed_nanosec()
 */
PJ_DECL(pj_uint32_t) pj_elapsed_usec( const pj_timestamp *start,
                                      const pj_timestamp *stop );

/**
 * 以32位纳秒为单位计算经过的时间。
 * 此函数使用当前平台可用的最高精度计算来计算运行时间，并考虑是否有浮点或64位精度算法可用。
 * 为了获得最大的可移植性，应用程序应该更喜欢使用这个函数，而不是自己计算运行时间
 *
 * @param start     开始时间戳
 * @param stop      结束时间戳
 *
 * @return	    运行时间（纳秒）
 *
 * @see pj_elapsed_time(), pj_elapsed_cycle(), pj_elapsed_usec()
 */
PJ_DECL(pj_uint32_t) pj_elapsed_nanosec( const pj_timestamp *start,
                                         const pj_timestamp *stop );

/**
 * 以32位周期计算经过的时间。
 * 此函数使用当前平台可用的最高精度计算来计算运行时间，并考虑是否有浮点或64位精度算法可用。
 * 为了获得最大的可移植性，应用程序应该更喜欢使用这个函数，而不是自己计算运行时间
 *
 * @param start     开始时间戳
 * @param stop      结束时间戳
 *
 * @return	    以周期为单位的运行时间
 *
 * @see pj_elapsed_usec(), pj_elapsed_time(), pj_elapsed_nanosec()
 */
PJ_DECL(pj_uint32_t) pj_elapsed_cycle( const pj_timestamp *start,
                                       const pj_timestamp *stop );


#endif	/* PJ_HAS_HIGH_RES_TIMER */

/** @} */


/* **************************************************************************/
/**
 * @defgroup PJ_APP_OS 应用程序执行
 * @ingroup PJ_OS
 * @{
 */

/**
 * 应用程序主功能的类型
 */
typedef int (*pj_main_func_ptr)(int argc, char *argv[]);

/**
 * 运行应用程序。这个函数必须在主线程中调用，在根据提供的标志进行必要的初始化之后，它将调用main_func()函数
 *
 * @param main_func 应用程序的主函数
 * @param argc	    main（）函数的参数数，将传递给main_func()
 * @param argv	    main（）函数的参数，将传递给main_func()
 * @param flags     应用程序执行的标志，当前必须为 0
 *
 * @return          main_func()的返回值
 */
PJ_DECL(int) pj_run_app(pj_main_func_ptr main_func, int argc, char *argv[],
			unsigned flags);

/** @} */


/* **************************************************************************/
/**
 * 内部 PJLIB 函数初始化线程子系统
 * @return          成功返回 PJ_SUCCESS，否则返回相应的错误码
 */
pj_status_t pj_thread_init(void);


PJ_END_DECL

#endif  /* __PJ_OS_H__ */

