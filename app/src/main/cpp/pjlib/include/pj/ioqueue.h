/** 已完成
 * I/O 调度机制  			执行网络I/O 和通信的API 构建块
 * 	procator 模式：
 * 		允许提交一个异步操作，完成后得到通知
 *
 * 	I/O 队列实现：
 * 		1. select() 	数量限制为 64，效率最低
 * 		2。 epoll() (Linux)  select() 更快替代品，没有数量限制
 *
 * 	并发规则：
 * 		1. 当有多个线程轮询 ioqueue 并且有多个句柄发出信号时，多个线程将同时执行回调以服务于事件。当事件发生在两个不同的句柄上时，这些并行执行是完全安全的。
 * 		2. 使用多线程时，当多个事件发生在同一个句柄上，或者当事件发生在一个句柄上（并且正在执行回调）并且应用程序同时对句柄执行注销时，必须小心
 *
 * 	并发设置：
 * 		pj_ioqueue_set_concurrency() 函数，可以在每个句柄（键）的基础上设置并发性。句柄的默认密钥并发值继承自 ioqueue 的密钥并发设置
 * 		pj_ioqueue_set_default_concurrency() 更改 ioqueue 的密钥并发设置。ioqueue 本身的默认密钥并发设置由编译时设置 PJ_IOQUEUE_DEFAULT_ALLOW_CONCURRENCY控制
 *
 * 		注意：
 * 			此密钥并发设置仅控制是否允许多线程同时对同一密钥进行操作。ioqueue 本身始终允许多个线程同时进入ioqeue，并且始终允许同时回调不同的键，而不管键的并发设置如何。
 *
 *	同一句柄的并行回调执行：
 *		密钥并发时（即允许对同一个密钥进行并行回调调用；这是默认设置）。
 *		当同一个键调用了多个挂起的操作时，ioqueue 只对该键同时执行回调。
 *		当密钥上只有一个挂起的操作时，发生在同一密钥上的事件将由 ioqueue 排队，因此不会同时执行回调调用。
 *
 *	启用并发（默认值）
 *		pj_ioq_allow_concur
 *		当一个线程发出注销时，另一个线程不会同时调用对同一个键的回调
 *
 *	禁用并发
 *		三种方式控制密钥并发设置：
 *			使用 pj_ioqueue_set_concurrency() 对每个句柄/密钥进行控制。
 * 			使用 pj_ioqueue_set_default_concurrency() 更改 ioqueue 上的默认密钥并发设置。
 * 			PJ_IOQUEUE_DEFAULT_ALLOW_CONCURRENCY 宏声明为零
 *
 *
 * 	宏：
 * 		ioqueue 单个轮询周期处理最大事件数		PJ_IOQUEUE_MAX_EVENTS_IN_SINGLE_POLL		16
 * 		每个轮询线程收集的最大候选事件数
 *
 * 	生命周期：
 * 		1. 初始化 I/O 队列
 * 		2. 设置 I/O 队列要使用的锁对象		//在句柄注册到 I/O 队列之前的调用	备用锁
 * 		3. 设置并发策略			只会影响后续的密钥注册，修改单个密钥的并发设置，使用 pj_ioqueue_set_concurrency()
 * 		4. 注册到 I/O 队列
 * 		5. 操作
 * 		6. 反注册 I/O 队列
 * 		7. 销毁 I/O 队列
 *
 * 		操作：
 * 			1. 将完成状态后处理到指定的操作键，并调用相应的回调
 * 			2. 接收指定 socket 的传入连接
 * 			3. socket 连接
 * 			4. 轮询 I/O 队列中已完成的事件
 * 			5. 读取 I/O 队列		recv / recvfrom
 * 			6. 写入 I/O 队列		send / sendto
 *
 * 	属性：
 * 		1. user_data
 * 		2. concurrency		并发策略
 * 		3. op_key
 *
 * 	额外：
 * 		1. 获取密钥的互斥（加锁）/trylock
 * 		2. 解锁 unlock
 * 		3. 检查是否挂起 (key+op_key)
 *
 *
 *
 */
#ifndef __PJ_IOQUEUE_H__
#define __PJ_IOQUEUE_H__

/**
 * @file ioqueue.h
 * @brief I/O 调度机制
 */

#include <pj/types.h>

PJ_BEGIN_DECL

/**
 * @defgroup PJ_IO Input/Output
 * @brief Input/Output
 * @ingroup PJ_OS
 *
 * 本节包含用于执行网络I/O和通信的API构建块。如果提供：
 *  - @ref PJ_SOCK
 *\n
 * 一个高度可移植的socket抽象，运行在各种网络API上，
 * 如标准 bsd socket、Windows socket、Linux kernel socket、PalmOS网络API等。
 *
 *  - @ref pj_addr_resolve
 *\n
 * 	可移植地址解析，实现 pj_gethostbyname()
 *
 *  - @ref PJ_SOCK_SELECT
 *\n
 *    一个可移植的、类似 select() 的API（ pj_sock_select()），可以用各种后端实现。
 *
 *  - @ref PJ_IOQUEUE
 *\n
 *  调度网络事件的框架
 *
 * 有关更多信息，请参阅下面的模块:
 */

/**
 * @defgroup PJ_IOQUEUE IOQueue: 基于 Proactor 模式的 I/O 事件调度
 * @ingroup PJ_IO
 * @{
 *
 * I/O 队列提供用于执行异步 I/O操作的API。它符合 proactor 模式，允许应用程序提交一个异步操作，并在操作完成后得到通知。
 *
 * I/O队列可以在套接字和文件描述符上工作。但是，对于异步文件操作，必须确保使用正确的文件I/O后端，因为并非所有文件I/O后端都可以与 ioqueue 一起使用。
 * 有关详细信息，请参阅 PJ_FILE_IO
 *
 * 该框架在异步操作API存在的平台上本机工作，例如windows NT的 IoCompletionPort/IOCP。在其他平台中，I/O 队列抽象了操作系统的事件轮询API，
 * 以提供类似于IoCompletionPort的语义，并且惩罚最小（即每个ioqueue和每个句柄互斥保护）。
 *
 * I/O队列提供的不仅仅是统一的抽象。它还包括：
 * 	-确保操作使用最有效的方法来利用底层机制，以在给定平台上实现最大的理论吞吐量。
 * 	-在给定平台上选择最有效的事件轮询机制。
 *
 *
 * 目前，I/O队列是通过以下方式实现的：
 * 	- select() 作为公分母，但效率最低。另外，描述符的数量限制为 PJ_IOQUEUE_MAX_HANDLES（缺省情况下为64）。
 * 	- Linux（用户模式和内核模式）上的 dev/epoll，它是Linux上 select（）的一个更快的替代品（更重要的是没有对描述符数量的限制）。
 * 	- Windows NT/2000/XP上的I/O完成端口，这是在基于 Windows NT的操作系统中调度事件的最有效方法，而且最重要的是，它没有要监视多少句柄的限制。
 * 	而且它也适用于文件（不仅仅是套接字）。
 *
 * \section pj_ioqueue_concurrency_sec 并发规则
 *
 * ioqueue 经过了微调，允许多个线程同时轮询句柄，以便在多处理器系统上运行应用程序时最大限度地提高可伸缩性。当有多个线程轮询 ioqueue 并且有多个句柄
 * 发出信号时，多个线程将同时执行回调以服务于事件。当事件发生在两个不同的句柄上时，这些并行执行是完全安全的。
 *
 * 但是，使用多线程时，当多个事件发生在同一个句柄上，或者当事件发生在一个句柄上（并且正在执行回调）并且应用程序同时对句柄执行注销时，必须小心。
 *
 * 根据应用于句柄的并发设置，上述场景的处理方式有所不同。
 *
 * \subsection pj_ioq_concur_set 句柄的并发设置
 *
 * 通过使用 pj_ioqueue_set_concurrency() 函数，可以在每个句柄（键）的基础上设置并发性。句柄的默认密钥并发值继承自 ioqueue 的密钥并发设置，
 * 可以使用 pj_ioqueue_set_default_concurrency() 更改 ioqueue 的密钥并发设置。ioqueue 本身的默认密钥并发设置由编译时设置 PJ_IOQUEUE_DEFAULT_ALLOW_CONCURRENCY控制。
 *
 * 请注意，此密钥并发设置仅控制是否允许多线程同时对同一密钥进行操作。ioqueue 本身始终允许多个线程同时进入ioqeue，
 * 并且始终允许同时回调不同的键，而不管键的并发设置如何。
 *
 * \subsection pj_ioq_parallel 同一句柄的并行回调执行
 *
 * 请注意，当启用了密钥并发时（即允许对同一个密钥进行并行回调调用；这是默认设置），当同一个键调用了多个挂起的操作时，
 * ioqueue 只对该键同时执行回调。例如，可以在同一个键上多次调用 pj_ioqueue_recvfrom()，每个键具有相同的键，但
 * 操作键不同（pj_ioqueue_op_key_t）。在这种情况下，当多个数据包同时到达该密钥时，多个线程可以同时执行回调，每个线
 * 程使用相同的密钥但操作密钥不同。
 *
 * 当密钥上只有一个挂起的操作（例如，在密钥上只有一个pj_ioqueue_recvfrom() 调用）时，发生在同一密钥上的事件
 * 将由 ioqueue 排队，因此不会同时执行回调调用。
 *
 * \subsection pj_ioq_allow_concur 已启用并发（默认值）
 *
 * ioqueue 的默认设置是允许多个线程对同一个句柄/键执行回调。选择此设置是为了提高应用程序的良好性能和可伸缩性。
 *
 * 但是，此设置在同步方面有一个主要缺点，应用程序必须仔细遵循以下准则，以确保对密钥的并行访问不会导致问题：
 * 	- 请注意，可能会同时为同一个键调用回调。
 * 	- 从 ioqueue 注销密钥时必须小心。应用程序必须注意，当一个线程发出注销时，另一个线程不会同时调用对同一个键的回调。
 * 	发生这种情况的原因是ioqueue函数使用的是指向键的指针，并且存在一种可能的争用情况，即在 ioqueue 有机会获取该指针上的互斥锁之前，
 * 	该指针已被其他线程呈现为无效
 *
 * \subsection pj_ioq_disallow_concur 已禁用并发
 *
 * 或者，应用程序可以禁用密钥并发以使同步更容易。如上所述，有三种方法可以控制密钥并发设置：
 * 	-通过使用 pj_ioqueue_set_concurrency() 对每个句柄/密钥进行控制。
 * 	-通过使用 pj_ioqueue_set_default_concurrency() 更改 ioqueue 上的默认密钥并发设置。
 * 	通过更改编译时的默认并发性，在 config_site.h 中将 PJ_IOQUEUE_DEFAULT_ALLOW_CONCURRENCY 宏声明为零
 *
 * \section pj_ioqeuue_examples_sec 例子
 *
 * 有关如何使用I/O队列的一些示例，请参见：
 *
 *  - \ref page_pjlib_ioqueue_tcp_test
 *  - \ref page_pjlib_ioqueue_udp_test
 *  - \ref page_pjlib_ioqueue_perf_test
 */


/**
 * 此结构描述在执行异步操作时要提交到 I/O 队列的操作特定密钥。调用完成回调时，此键将返回到应用程序。
 *
 * 应用程序通常希望将其特定的数据附加到 user_data 字段中，以便在调用回调时跟踪哪个操作已完成。或者，
 * 应用程序还可以扩展此结构以包含其数据，因为在完成回调中返回的指针与调用异步函数时提供的指针完全相同。
 *
 */
typedef struct pj_ioqueue_op_key_t
{ 
    void *internal__[32];           /**< 内部 I/O 队列数据.   */
    void *activesock_data;	    /**< 活动socket data	    */
    void *user_data;                /**< 应用数据          */
} pj_ioqueue_op_key_t;

/**
 * 此结构描述I/O操作完成时要调用的回调
 */
typedef struct pj_ioqueue_callback
{
    /**
     * pj_ioqueue_recv 或 pj_ioqueue_recvfrom 完成时回调
     *
     * @param key	    key
     * @param op_key        操作 key
     * @param bytes_read    >=0表示读取的数据量，否则为负值，包含错误代码。
     * 要获取pj_status_t 错误代码，请使用(pj_status_t code = -bytes_read)
     */
    void (*on_read_complete)(pj_ioqueue_key_t *key, 
                             pj_ioqueue_op_key_t *op_key, 
                             pj_ssize_t bytes_read);

    /**
     * pj_ioqueue_send pj_ioqueue_sendto 完成时回调
     *
     * @param key	    	key.
     * @param op_key        操作 key
     * @param bytes_sent    >=0表示入的数据量，否则为负值，包含错误代码。
     * 要获取pj_status_t 错误代码，请使用(pj_status_t code = -bytes_read)
     */
    void (*on_write_complete)(pj_ioqueue_key_t *key, 
                              pj_ioqueue_op_key_t *op_key, 
                              pj_ssize_t bytes_sent);

    /**
     * 调用pj_ioqueue_accept 完成时回调
     *
     * @param key	    The key.
     * @param op_key        操作 key.
     * @param sock          新连接的 socket
     * @param status	    操作完成返回 0
     */
    void (*on_accept_complete)(pj_ioqueue_key_t *key, 
                               pj_ioqueue_op_key_t *op_key, 
                               pj_sock_t sock, 
                               pj_status_t status);

    /**
     * 调用 pj_ioqueue_connect 完成时回调
     *
     * @param key	    The key.
     * @param status	    操作成功时返回 PJ_SUCCESS
     */
    void (*on_connect_complete)(pj_ioqueue_key_t *key, 
                                pj_status_t status);
} pj_ioqueue_callback;


/**
 * 挂起的I/O队列操作的类型。此枚举仅在ioqueue内部使用。
 */
typedef enum pj_ioqueue_operation_e
{
    PJ_IOQUEUE_OP_NONE		= 0,	/**< 无操作          */
    PJ_IOQUEUE_OP_READ		= 1,	/**< read() 操作    */
    PJ_IOQUEUE_OP_RECV          = 2,    /**< recv() 操作      */
    PJ_IOQUEUE_OP_RECV_FROM	= 4,	/**< recvfrom() 操作  */
    PJ_IOQUEUE_OP_WRITE		= 8,	/**< write() 操作     */
    PJ_IOQUEUE_OP_SEND          = 16,   /**< send() 操作      */
    PJ_IOQUEUE_OP_SEND_TO	= 32,	/**< sendto() 操作   */
#if defined(PJ_HAS_TCP) && PJ_HAS_TCP != 0
    PJ_IOQUEUE_OP_ACCEPT	= 64,	/**< accept() 操作   */
    PJ_IOQUEUE_OP_CONNECT	= 128	/**< connect() 操作   */
#endif	/* PJ_HAS_TCP */
} pj_ioqueue_operation_e;


/**
 * 此宏指定在支持 ioqueue 的实现上，ioqueue 可以在单个轮询周期处理的最大事件数。该值只有在 PJLIB 构建期间指定时才有意义。
 */
#ifndef PJ_IOQUEUE_MAX_EVENTS_IN_SINGLE_POLL
#   define PJ_IOQUEUE_MAX_EVENTS_IN_SINGLE_POLL     (16)
#endif


/**
 * 此宏指定每个轮询线程收集的最大候选事件数，以便在每个轮询周期中能够达到最大处理事件数
 * （即：PJ_IOQUEUE_MAX_EVENTS_IN_SINGLE_POLL）。
 *
 * 事件候选将作为事件分派给应用程序，除非它已经被其他轮询线程分派。因此，为了预测这样的竞争条件，
 * 每个poll操作收集的事件候选应该多于PJ_IOQUEUE_MAX_EVENTS_IN_SINGLE_POLL，建议的值是
 * （PJ_IOQUEUE_MAX_EVENTS_IN_SINGLE_POLL * 轮询线程数）。
 *
 * 该值仅在PJLIB构建期间指定时才有意义，并且仅在多个轮询线程环境中有效。
 */
#if !defined(PJ_IOQUEUE_MAX_CAND_EVENTS) || \
    PJ_IOQUEUE_MAX_CAND_EVENTS < PJ_IOQUEUE_MAX_EVENTS_IN_SINGLE_POLL
#   undef  PJ_IOQUEUE_MAX_CAND_EVENTS
#   define PJ_IOQUEUE_MAX_CAND_EVENTS	PJ_IOQUEUE_MAX_EVENTS_IN_SINGLE_POLL
#endif


/**
 * 当在 ioqueue 的 recv() 或 send() 操作中指定此标志时，ioqueue 将始终将操作标记为异步。
 */
#define PJ_IOQUEUE_ALWAYS_ASYNC	    ((pj_uint32_t)1 << (pj_uint32_t)31)

/**
 * 返回ioqueue实现的名称。
 *
 * @return		实现的名称
 */
PJ_DECL(const char*) pj_ioqueue_name(void);


/**
 * 创建新的I/O队列框架
 *
 * @param pool		分配I/O队列结构的池
 * @param max_fd	支持的最大句柄数，不应超过PJ_IOQUEUE_MAX_HANDLES
 * @param ioqueue	保存新创建的I/O队列的指针
 *
 * @return		成功返回 PJ_SUCCESS
 */
PJ_DECL(pj_status_t) pj_ioqueue_create( pj_pool_t *pool, 
					pj_size_t max_fd,
					pj_ioqueue_t **ioqueue);

/**
 * 销毁 I/O 队列
 *
 * @param ioque	        即将销毁的 I/O 队列
 *
 * @return              成功返回 PJ_SUCCESS
 */
PJ_DECL(pj_status_t) pj_ioqueue_destroy( pj_ioqueue_t *ioque );

/**
 * 设置I/O队列要使用的锁对象。此函数只能在创建I/O队列之后，在将任何句柄注册到I/O队列之前调用。
 *
 * 最初，I/O队列是使用非递归互斥保护创建的。应用程序可以通过调用此函数提供备用锁。
 *
 * @param ioque         The ioqueue instance.
 * @param lock          ioqueue要使用的锁。
 * @param auto_delete   非零，锁将被ioqueue删除。
 *
 * @return              成功返回 PJ_SUCCESS，否则返回适当的错误码
 */
PJ_DECL(pj_status_t) pj_ioqueue_set_lock( pj_ioqueue_t *ioque, 
					  pj_lock_t *lock,
					  pj_bool_t auto_delete );

/**
 * 为此 ioqueue 设置默认并发策略。如果未调用此函数，则 ioqueue 的默认并发策略由编译时设置PJ_IOQUEUE_DEFAULT_ALLOW_CONCURRENCY控制
 *
 * 请注意，将并发设置更改为 ioqueue 只会影响后续的密钥注册。要修改单个密钥的并发设置，请使用pj_ioqueue_set_concurrency()
 *
 * @param ioqueue	队列实例
 * @param allow		非零表示允许并发回调调用，PJ_FALSE表示不允许。
 *
 * @return		成功返回 PJ_SUCCESS，否则返回适当错误码
 */
PJ_DECL(pj_status_t) pj_ioqueue_set_default_concurrency(pj_ioqueue_t *ioqueue,
							pj_bool_t allow);

/**
 * 将套接字注册到I/O队列框架。
 * 当一个套接字注册到 IOQueue时，它可以被修改为使用非阻塞 IO。如果对其进行了修改，则不能保证在取消注册套接字后将恢复此修改。
 *
 * @param pool	    为指定的句柄分配资源，在从I/O队列注销句柄/密钥之前，该句柄必须有效
 * @param ioque	    I/O 队列
 * @param sock	    The socket.
 * @param user_data 要与密钥关联的用户数据，可以稍后检索。
 * @param cb	    I/O操作完成时调用的回调。
 * @param key       接收要与此套接字关联的密钥的指针。后续的I/O队列操作将需要此密钥
 *
 * @return	    成功返回 PJ_SUCCESS 否则错误码
 */
PJ_DECL(pj_status_t) pj_ioqueue_register_sock( pj_pool_t *pool,
					       pj_ioqueue_t *ioque,
					       pj_sock_t sock,
					       void *user_data,
					       const pj_ioqueue_callback *cb,
                                               pj_ioqueue_key_t **key );

/**
 * pj_ioqueue_register_sock() 的变体，带有附加的组锁参数。
 * 如果为密钥设置了组锁，则在注册套接字时，密钥将添加引用计数器，在销毁套接字时，密钥将减少引用计数器。
 */
PJ_DECL(pj_status_t) pj_ioqueue_register_sock2(pj_pool_t *pool,
					       pj_ioqueue_t *ioque,
					       pj_sock_t sock,
					       pj_grp_lock_t *grp_lock,
					       void *user_data,
					       const pj_ioqueue_callback *cb,
                                               pj_ioqueue_key_t **key );

/**
 * 从I/O队列框架注销。调用者在调用此函数之前，必须确保键没有任何挂起的操作，方法是为所有以前提交的操作（异步连接除外）
 * 调用 pj_ioqueue_is_pending()，必要时调用 pj_ioqueue_post_completion()，取消挂起的操作
 *
 * 请注意，异步连接操作将在注销期间自动取消。
 *
 * 另请注意，当使用I/O完成端口后端时，应用程序必须在注销密钥后立即关闭句柄。这是因为 IOCP 没有注销API。从IOCP注销句柄的唯一方法是关闭句柄。
 *
 * @param key	    以前从注册中获得的密钥。
 *
 * @return          成功返回PJ_SUCCESS，否则返回错误码
 *
 * @see pj_ioqueue_is_pending
 */
PJ_DECL(pj_status_t) pj_ioqueue_unregister( pj_ioqueue_key_t *key );


/**
 * 获取与ioqueue键关联的用户数据。
 *
 * @param key	    以前从注册中获得的密钥。
 *
 * @return          与描述符相关联的用户数据，如果出现错误或注册期间没有数据与密钥相关联，则返回NULL
 */
PJ_DECL(void*) pj_ioqueue_get_user_data( pj_ioqueue_key_t *key );

/**
 * 设置或更改要与文件描述符、句柄或套接字描述符关联的用户数据
 *
 * @param key	    以前从注册中获得的密钥
 * @param user_data 要与描述符关联的用户数据
 * @param old_data  用于检索旧用户数据的可选参数
 *
 * @return          成功返回 PJ_SUCCESS，失败返回错误码
 */
PJ_DECL(pj_status_t) pj_ioqueue_set_user_data( pj_ioqueue_key_t *key,
                                               void *user_data,
                                               void **old_data);

/**
 * 配置是否允许 ioqueue 并发/并行调用 key 的回调。键的默认并发设置由 ioqueue 的默认并发值控制，该值可以通过调用
 * pj_ioqueue_set_default_concurrency() 进行更改。
 *
 * 如果该键允许并发，则意味着如果有多个挂起操作同时完成，则多个线程可能同时调用该键的回调。这通常会促进应用程序良好的可伸缩性，
 * 但会牺牲管理应用程序代码中并发访问的复杂性。
 *
 * 或者，应用程序可以通过将 allow标志设置为false来禁用并发访问。禁用并发时，一次只能有一个线程调用键的回调。
 *
 * @param key	    以前从注册中获得的密钥
 * @param allow	    将其设置为非零值以允许并发回调调用，将其设置为零(PJ_FALSE)
 *
 * @return	    成功返回 PJ_SUCCESS，否则返回适当错误码
 */
PJ_DECL(pj_status_t) pj_ioqueue_set_concurrency(pj_ioqueue_key_t *key,
						pj_bool_t allow);

/**
 * 获取密钥的互斥。当密钥的并发性被禁用时，应用程序可以调用此函数以使其操作与密钥的回调同步（即，此函数将阻塞，直到密钥的回调返回）。
 *
 * @param key	    以前从注册中获得的密钥
 *
 * @return	    成功时返回 PJ_SUCCESS ，否则适当的错误码
 */
PJ_DECL(pj_status_t) pj_ioqueue_lock_key(pj_ioqueue_key_t *key);

/**
 * 尝试获取密钥的互斥。当密钥的并发性被禁用时，应用程序可以调用此函数以使其操作与密钥的回调同步
 *
 * @param key	    以前从注册中获得的密钥
 *
 * @return	    成功返回PJ_SUCCESS，否则返回适当的错误码
 */
PJ_DECL(pj_status_t) pj_ioqueue_trylock_key(pj_ioqueue_key_t *key);

/**
 * 释放先前用 pj_ioqueue_lock_key() 获取的锁
 *
 * @param key	    以前从注册中获得的密钥。
 *
 * @return	    成功返回PJ_SUCCESS，否则返回适当的错误码
 */
PJ_DECL(pj_status_t) pj_ioqueue_unlock_key(pj_ioqueue_key_t *key);

/**
 * 初始化操作 key.
 *
 * @param op_key    被初始化的操作 key
 * @param size	    操作key 的大小
 */
PJ_DECL(void) pj_ioqueue_op_key_init( pj_ioqueue_op_key_t *op_key,
				      pj_size_t size );

/**
 * 检查指定的操作密钥上的操作是否挂起。
 * op_key 必须已用 pj_ioqueue_op_key_init() 初始化，或在初始化之前作为挂起操作提交，否则结果未定义。
 *
 * @param key       The key.
 * @param op_key    操作键，以前提交给任何I/O函数，并已返回PJ_EPENDING
 *
 * @return          非零表示仍挂起
 */
PJ_DECL(pj_bool_t) pj_ioqueue_is_pending( pj_ioqueue_key_t *key,
                                          pj_ioqueue_op_key_t *op_key );


/**
 * 将完成状态后处理到指定的操作键，并调用相应的回调。调用回调时，将从 bytes_status 参数设置读/写回调中接收的字节数或accept/connect回调中的状态。
 *
 * @param key           The key.
 * @param op_key        挂起的操作 key
 * @param bytes_status  要设置的字节数或状态。这里有一个很好的值 -PJ_ECANCELLED
 *
 * @return              如果已成功发送完成状态，则返回PJ_SUCCESS
 */
PJ_DECL(pj_status_t) pj_ioqueue_post_completion( pj_ioqueue_key_t *key,
                                                 pj_ioqueue_op_key_t *op_key,
                                                 pj_ssize_t bytes_status );



#if defined(PJ_HAS_TCP) && PJ_HAS_TCP != 0
/**
 * 指示I/O队列接受指定侦听套接字上的传入连接。无论连接是否立即可用，此函数都将立即返回（即非阻塞）。
 * 如果函数不能立即完成，调用方在调用 pj_ioqueue_poll() 时将收到有关传入连接的通知。如果一个新的
 * 连接立即可用，函数返回PJ_SUCCESS；在这种情况下，将不调用回调。
 *
 * @param key	    注册到服务器套接字的密钥
 * @param op_key    要与挂起操作关联的操作特定键，以便应用程序在调用回调时可以跟踪哪个操作已完成
 * @param new_sock  参数，该参数包含接收传入连接的新套接字的指针
 * @param local	    包含指向接收本地地址的变量指针的可选参数
 * @param remote    包含指向接收远程地址的变量指针的可选参数
 * @param addrlen   输入时，包含地址缓冲区的长度，输出时，包含地址的实际长度。此参数是可选的
 * @return
 *  - PJ_SUCCESS    当连接立即可用时，参数将更新为包含有关新连接的信息。在这种情况下，不会调用完成回调。
 *  - PJ_EPENDING   如果没有可用的连接。当新连接到达时，将调用回调
 *  - non-zero      表示相应的错误代码。
 */
PJ_DECL(pj_status_t) pj_ioqueue_accept( pj_ioqueue_key_t *key,
                                        pj_ioqueue_op_key_t *op_key,
					pj_sock_t *new_sock,
					pj_sockaddr_t *local,
					pj_sockaddr_t *remote,
					int *addrlen );

/**
 * 启动非阻塞套接字连接。如果无法立即连接套接字，则会安排异步connect()，并在调用 pj_ioqueue_poll() 时通过完成回调通知调用方。
 * 如果立即连接 socket，则函数返回PJ_SUCCESS，并且不会调用完成回调。
 *
 * @param key	    与TCP套接字关联的密钥
 * @param addr	    远端地址
 * @param addrlen   远端地址长度
 *
 * @return
 *  - PJ_SUCCESS    如果立即连接。在这种情况下，不会调用完成回调
 *  - PJ_EPENDING   如果操作已排队
 *  - non-zero      错误码
 */
PJ_DECL(pj_status_t) pj_ioqueue_connect( pj_ioqueue_key_t *key,
					 const pj_sockaddr_t *addr,
					 int addrlen );

#endif	/* PJ_HAS_TCP */

/**
 * 轮询I/O队列中已完成的事件
 *
 * 注意：在Symbian中不需要轮询ioqueue。有关详细信息，请参阅 @ref PJ_SYMBIAN_OS
 *
 * @param ioque		I/O 队列.
 * @param timeout	轮询超时，如果线程希望不确定地等待事件，则为NULL
 *
 * @return 
 *  - zero 超时（没有事件）
 *  - (<0) 如果在轮询期间发生错误。不会调用回调
 *  - (>1) 表示事件的数量。已调用回调
 */
PJ_DECL(int) pj_ioqueue_poll( pj_ioqueue_t *ioque,
			      const pj_time_val *timeout);


/**
 * 指示I/O队列读取指定的句柄。无论是否传输了某些数据，此函数都会立即返回（即非阻塞）。如果操作不能立即完成，
 * 调用方在调用 pj_ioqueue_poll() 时将收到完成通知。如果数据立即可用，函数将返回PJ_SUCCESS，并且不会调用回调。
 *
 * @param key	    唯一标识句柄的 key
 * @param op_key    要与挂起操作关联的操作特定键，以便应用程序在调用回调时可以跟踪哪个操作已完成。调用者必须确保此键在函数完成之前保持有效
 * @param buffer    保存读取数据的缓冲区。调用方必须确保此缓冲区在框架完成读取句柄之前保持有效
 * @param length    在输入时，它指定缓冲区的大小。如果可以立即读取数据，则函数返回PJ_SUCCESS，并用读取的数据量填充此参数。如果函数处
 * 于挂起状态，则会通知调用者回调中读取的数据量。此参数可以指向调用者堆栈中的局部变量，并且在挂起操作期间不必保持有效
 * @param flags     接收标志。如果标志有 PJ_IOQUEUE_ALWAYS_ASYNC ，那么函数将永远不会返回PJ_SUCCESS
 *
 * @return
 *  - PJ_SUCCESS    如果在缓冲区中接收到即时数据。在这种情况下，将不调用回调
 *  - PJ_EPENDING   如果操作已排队，则在收到数据时将调用回调
 *  - non-zero      返回值表示错误代码。
 */
PJ_DECL(pj_status_t) pj_ioqueue_recv( pj_ioqueue_key_t *key,
                                      pj_ioqueue_op_key_t *op_key,
				      void *buffer,
				      pj_ssize_t *length,
				      pj_uint32_t flags );

/**
 * 此函数的行为类似于 pj_ioqueue_recv()，只是它通常为socket调用，远程地址也将随数据一起返回。
 * 调用方必须确保buffer和addr都保持有效，直到框架完成数据读取。
 *
 * @param key	    唯一标识句柄的 key
 * @param op_key    要与挂起操作关联的操作 key，以便应用程序在调用回调时可以跟踪哪个操作已完成
 * @param buffer    保存读取数据的缓冲区。调用方必须确保此缓冲区在框架完成读取句柄之前保持有效
 * @param length	在输入时，它指定缓冲区的大小。如果可以立即读取数据，则函数返回PJ_SUCCESS，
 * 并用读取的数据量填充此参数。如果函数处于挂起状态，则会通知调用者回调中读取的数据量。此参数可以指
 * 向调用者堆栈中的局部变量，并且在挂起操作期间不必保持有效。
 * @param flags     接收标志。如果标志有PJ_IOQUEUE_ALWAYS_ASYNC，那么函数将永远不会返回PJ_SUCCESS
 * @param addr      指向接收地址的缓冲区的可选指针
 * @param addrlen   输入时，指定地址缓冲区的长度。
 * 					输出时，它将用地址的实际长度填充。如果未指定 addr，则此参数可以为空。
 *
 * @return
 *  - PJ_SUCCESS    如果收到即时数据。在这种情况下，回调必须在函数返回之前被调用，并且不会安排任何挂起的操作
 *  - PJ_EPENDING   如果操作已排队
 *  - non-zero      返回值表示错误代码
 */
PJ_DECL(pj_status_t) pj_ioqueue_recvfrom( pj_ioqueue_key_t *key,
                                          pj_ioqueue_op_key_t *op_key,
					  void *buffer,
					  pj_ssize_t *length,
                                          pj_uint32_t flags,
					  pj_sockaddr_t *addr,
					  int *addrlen);

/**
 * 指示I/O队列写入句柄。无论是否传输了某些数据，此函数都将立即返回（即非阻塞）。如果函数不能立即完成，
 * 调用方在调用 pj_ioqueue_poll() 时会收到完成通知。如果操作立即完成并且数据已被传输，则函数返回
 * PJ_SUCCESS，并且不会调用回调
 *
 * @param key	    标识句柄的 key
 * @param op_key    要与挂起操作关联的操作特定键，以便应用程序在调用回调时可以跟踪哪个操作已完成
 * @param data	    要发送的数据。调用方必须确保此缓冲区在写入操作完成之前保持有效
 * @param length    输入时，它指定要发送的数据长度。当数据被立即发送时，此函数返回PJ_SUCCESS，
 * 此参数包含发送的数据长度。如果无法立即发送数据，则会安排一个异步操作，并通过回调通知调用者发送的字节数。
 * 此参数可以指向调用方堆栈上的局部变量，并且在操作完成之前不必保持有效
 * @param flags     发送标志。如果标志有PJ_IOQUEUE_ALWAYS_ASYNC，那么函数将永远不会返回PJ_SUCCESS
 *
 * @return
 *  - PJ_SUCCESS    如果数据被立即传输。在这种情况下，没有计划任何挂起的操作，也不会调用回调
 *  - PJ_EPENDING   如果操作已排队。一旦数据库被传输，回调将被调用
 *  - non-zero      返回值表示错误代码
 */
PJ_DECL(pj_status_t) pj_ioqueue_send( pj_ioqueue_key_t *key,
                                      pj_ioqueue_op_key_t *op_key,
				      const void *data,
				      pj_ssize_t *length,
				      pj_uint32_t flags );


/**
 * 指示I/O队列写入句柄。无论是否传输了某些数据，此函数都将立即返回（即非阻塞）。如果函数不能立即完成，
 * 调用方在调用 pj_ioqueue_poll() 时会收到完成通知。如果操作立即完成并且数据已被传输，则函数返回PJ_SUCCESS，并且不会调用回调。
 *
 * @param key	    标识句柄的键。
 * @param op_key    要与挂起操作关联的操作特定键，以便应用程序在调用回调时可以跟踪哪个操作已完成
 * @param data	    要发送的数据。调用方必须确保此缓冲区在写入操作完成之前保持有效
 * @param length    输入时，它指定要发送的数据长度。当数据被立即发送时，此函数返回PJ_SUCCESS，此参数包含发送的数据长度。如果无法
 * 立即发送数据，则会安排一个异步操作，并通过回调通知调用者发送的字节数。此参数可以指向调用方堆栈上的局部变量，并且在操作完成之前不必保
 * 持有效
 * @param flags     发送标志。如果标志有PJ_IOQUEUE_ALWAYS_ASYNC，那么函数将永远不会返回PJ_SUCCESS
 * @param addr      可选的远端地址
 * @param addrlen   指定了远程地址长度
 *
 * @return
 *  - PJ_SUCCESS    如果数据被立即写入。
 *  - PJ_EPENDING   如果操作已排队。
 *  - non-zero      返回值表示错误代码。
 */
PJ_DECL(pj_status_t) pj_ioqueue_sendto( pj_ioqueue_key_t *key,
                                        pj_ioqueue_op_key_t *op_key,
					const void *data,
					pj_ssize_t *length,
                                        pj_uint32_t flags,
					const pj_sockaddr_t *addr,
					int addrlen);


/**
 * !}
 */

PJ_END_DECL

#endif	/* __PJ_IOQUEUE_H__ */

