/**
 * 已完成
 *
 * active socket:
 * 		ioqueue 的作用及其关系，同步和异步执行
 * 		回调函数：
 * 			on_data_read
 * 			on_data_recvfrom
 * 			on_data_sent
 * 			on_accept_complete
 * 			on_accept_complete2
 * 			on_connect_complete
 *
 */
#ifndef __PJ_ASYNCSOCK_H__
#define __PJ_ASYNCSOCK_H__

/**
 * @file activesock.h
 * @brief 活动 socket
 */

#include <pj/ioqueue.h>
#include <pj/sock.h>


PJ_BEGIN_DECL

/**
 * @defgroup PJ_ACTIVESOCK 活动 socket I/O
 * @brief Active socket performs active operations on socket.
 * @ingroup PJ_IO
 * @{
 *
 * 活动套接字是 ioqueue 的更高级别抽象。它提供了套接字操作的自动化，否则这些操作必须由应用程序手动完成。
 * 例如，对于socket recv()、recvfrom()和accept()操作，应用程序只需要调用这些操作一次，并且每当数
 * 据或传入的TCP连接（在accept()的情况下）到达时，应用程序就会收到通知。
 */

/**
 * 描述 active socket 的结构体
 */
typedef struct pj_activesock_t pj_activesock_t;

/**
 * 活动socket 的回调结构
 */
typedef struct pj_activesock_cb
{
    /**
     * pj_activesock_start_read() 结果的回调
     *
     * @param 活动的 socket
     * @param data	包含新数据（如果有的话）的缓冲区。如果status参数为non-PJ_SUCCESS，则此参数可能为NULL
     * @param size
     * @param status	读取操作的状态。这可能包含 non-PJ_SUCCESS，例如当TCP连接已关闭时。在这种情况下，缓冲区可能
     * 包含应用程序可能要处理的上一次回调的剩余数据。
     * @param remainder		如果应用程序希望在缓冲区中保留一些数据（对于TCP应用程序来说很常见），它应该将剩余数据移到缓冲区的前面部分，
     * 并在这里设置剩余长度。对于UDP socket，此参数的值将被忽略。
     *
     * @return		如果需要进一步读取，则为 PJ_TRUE；如果应用程序不再需要接收，则为 PJ_FALSE。应用程序可能在这次回调中销毁活动的 socket ,
     * 则返回PJ_FALSE
     */
    pj_bool_t (*on_data_read)(pj_activesock_t *asock,
			      void *data,
			      pj_size_t size,
			      pj_status_t status,
			      pj_size_t *remainder);
    /**
     * pj_activesock_start_recvfrom() 有数据包到达时回调
     *
     * @param asock	活动的 socket
     * @param data	包含数据包的缓冲区（如果有的话）。如果status参数为non-PJ_SUCCESS，则此参数将设置为NULL
     * @param size	数据包的缓冲区的长度，如果status 为 no-PJ_SUCCESS，则为 0
     * @param src_addr	数据包的源地址
     * @param addr_len	源地址的长度
     * @param status	状态值
     *
     * @return		如果想进一步的读取数据，则返回 PJ_TRUE；否则返回 PJ_FALSE。本次回调中销毁活动的 socket 也返回 PJ_FALSE
     */
    pj_bool_t (*on_data_recvfrom)(pj_activesock_t *asock,
				  void *data,
				  pj_size_t size,
				  const pj_sockaddr_t *src_addr,
				  int addr_len,
				  pj_status_t status);

    /**
     * 数据已经发送时回调
     *
     * @param asock	The active socket.
     * @param send_key	与发送相关的密钥
     * @param sent	如果值非零，则表示发送的数据数。当值为负数时，它包含可通过对值求反来检索的错误代码（即status=-sent）。
     *
     * @return		本次回调，应用可能销毁这个活动的 socket，则返回 PJ_FALSE
     */
    pj_bool_t (*on_data_sent)(pj_activesock_t *asock,
			      pj_ioqueue_op_key_t *send_key,
			      pj_ssize_t sent);

    /**
     * 当新连接作为 pj_activesock_start_accept() 的结果到达时调用此回调。如果需要 accept操作的状态，
     * 请使用 on_accept_complete2 而不是此回调。
     *
     * @param asock	活动的socket
     * @param newsock	新来的 socket
     * @param src_addr  连接的源地址
     * @param addr_len	源地址的长度
     *
     * @return		如果需要进一步accept（），则为PJ_TRUE；如果应用程序不再希望接受传入连接，则为PJ_FALSE。
     * 应用程序可能会破坏回调中的活动套接字，并在此处返回PJ_FALSE
     */
    pj_bool_t (*on_accept_complete)(pj_activesock_t *asock,
				    pj_sock_t newsock,
				    const pj_sockaddr_t *src_addr,
				    int src_addr_len);

    /**
     * 当新连接作为pj_activesock_start_accept()的结果到达时调用此回调。
     *
     * @param asock	活动的socket
     * @param newsock	新来的 socket
     * @param src_addr  连接的源地址
     * @param addr_len	源地址的长度
     * @param status	接受操作的状态。这可能 non-PJ_SUCCESS，例如当TCP侦听器处于错误状态时，例如在iOS平台上，应用程序从后台唤醒后。
     *
     * @return		如果需要进一步accept()，则为PJ_TRUE；如果应用程序不再希望接受传入连接，则为PJ\u FALSE。应用程序可能会破坏回调中的活动套接字，并在此处返回PJ\u FALSE。
     * PJ_TRUE if further accept() is desired, and PJ_FALSE
     *			when application no longer wants to accept incoming
     *			connection. Application may destroy the active socket
     *			in the callback and return PJ_FALSE here.
     */
    pj_bool_t (*on_accept_complete2)(pj_activesock_t *asock,
				     pj_sock_t newsock,
				     const pj_sockaddr_t *src_addr,
				     int src_addr_len, 
				     pj_status_t status);

    /**
     * 当连接操作完成时，将调用此回调。
     *
     * @param asock	The active  socket.
     * @param status	连接结果，成功建立连接，则状态将包含PJ_SUCCESS
     *
     * @return		应用程序可能在回调中销毁活动 socket，并在此返回PJ_FALSE
     */
    pj_bool_t (*on_connect_complete)(pj_activesock_t *asock,
				     pj_status_t status);

} pj_activesock_cb;


/**
 * 活动 socket 配置，此结构需通过 pj_activesock_cfg_default() 获取
 */
typedef struct pj_activesock_cfg
{
    /**
     * ioqueue key的可选组锁
     */
    pj_grp_lock_t *grp_lock;

    /**
     * 活动套接字支持的并发异步操作数。此值仅影响套接字接收和接受操作--活动套接字将基于此字段的值确定发出一个或
     * 多个异步读取和接受操作。当 ioqueue 被多个线程轮询时，将此字段设置为多个将允许在多处理器系统上同时
     * 处理多个传入数据或传入连接。
     *
     * 默认值为 1
     */
    unsigned async_cnt;

    /**
     * 当套接字注册到 ioqueue 时，要在其上强制执行的 ioqueue 并发。有关ioqueue并发的详细信息，请参见 pj_ioqueue_set_concurrency()。
     * 当该值为-1时，不会强制此套接字的并发设置，并且套接字将继承 ioqueue 的并发设置。当该值为零时，活动套接字将禁用套接字的并发性。
     * 当该值为 +1 时，活动套接字将为套接字启用并发性。
     *
     * 默认值为-1。
     */
    int concurrency;

    /**
     * 如果指定了此选项，活动套接字将确保具有面向流的套接字的异步发送操作仅在发送所有数据后调用回调。这意味着活动套接字将
     * 自动重新发送剩余的数据，直到发送完所有数据。
     *
     * 请注意，指定此选项时，可能会在发送部分数据后报告错误。设置此选项还将禁用套接字的 ioqueue 并发性。
     *
     * 默认值为 1
     */
    pj_bool_t whole_data;

} pj_activesock_cfg;


/**
 * 使用默认值初始化活动套接字配置
 *
 * @param cfg		要初始化的配置
 */
PJ_DECL(void) pj_activesock_cfg_default(pj_activesock_cfg *cfg);


/**
 * 为指定的套接字创建活动套接字。这将把套接字注册到指定的 ioqueue
 *
 * @param pool		从中分配内存的池。
 * @param sock		socket 句柄
 * @param sock_type	指定套接字类型，可以是pj_SOCK_STREAM()或pj_SOCK_DGRAM()。活动套接字需要此信息来处理面向连接的套接字的连接关闭。
 * @param ioqueue	使用的 ioqueue
 * @param opt		可选设置。未指定此设置时，将使用默认值。
 * @param cb		指向包含应用程序回调的结构的指针
 * @param user_data	要与此活动套接字关联的任意用户数据
 * @param p_asock	接收活动套接字实例的指针
 *
 * @return		操作成功返回 PJ_SUCCESS，操作失败返回错误码
 */
PJ_DECL(pj_status_t) pj_activesock_create(pj_pool_t *pool,
					  pj_sock_t sock,
					  int sock_type,
					  const pj_activesock_cfg *opt,
					  pj_ioqueue_t *ioqueue,
					  const pj_activesock_cb *cb,
					  void *user_data,
					  pj_activesock_t **p_asock);

/**
 * 创建UDP套接字描述符，将其绑定到指定的地址，并为套接字描述符创建活动套接字。
 *
 *
 * @param pool		Pool to allocate memory from.
 * @param addr		参数为 NULL,假定为 AF_INET 则会绑定到任意地址或任意端口上
 * @param opt		选项设置，未指定会使用默认值
 * @param cb		指向包含应用程序回调的结构的指针
 * @param user_data	活动socket 的用户数据
 * @param p_asock	接收活动套接字实例的指针
 * @param bound_addr	如果指定了此参数，则返回时将用绑定地址填充
 *
 * @return		操作成功返回 PJ_SUCCESS，操作失败返回相应的错误码
 */
PJ_DECL(pj_status_t) pj_activesock_create_udp(pj_pool_t *pool,
					      const pj_sockaddr *addr,
					      const pj_activesock_cfg *opt,
					      pj_ioqueue_t *ioqueue,
					      const pj_activesock_cb *cb,
					      void *user_data,
					      pj_activesock_t **p_asock,
					      pj_sockaddr *bound_addr);

/**
 * 关闭活动 socket。将从 ioqueue 中注销套接字并最终关闭套接字。
 *
 * @param asock	    活动的 socket
 *
 * @return	    操作成功返回 PJ_SUCCESS，操作失败返回响应的错误码
 */
PJ_DECL(pj_status_t) pj_activesock_close(pj_activesock_t *asock);

#if (defined(PJ_IPHONE_OS_HAS_MULTITASKING_SUPPORT) && \
     PJ_IPHONE_OS_HAS_MULTITASKING_SUPPORT!=0) || \
     defined(DOXYGEN)
/**
 * Set iPhone OS background mode setting. Setting to 1 will enable TCP
 * active socket to receive incoming data when application is in the
 * background. Setting to 0 will disable it. Default value of this
 * setting is PJ_ACTIVESOCK_TCP_IPHONE_OS_BG.
 *
 * This API is only available if PJ_IPHONE_OS_HAS_MULTITASKING_SUPPORT
 * is set to non-zero.
 *
 * @param asock	    The active socket.
 * @param val	    The value of background mode setting.
 *
 */
PJ_DECL(void) pj_activesock_set_iphone_os_bg(pj_activesock_t *asock,
					     int val);

/**
 * Enable/disable support for iPhone OS background mode. This setting
 * will apply globally and will affect any active sockets created
 * afterwards, if you want to change the setting for a particular
 * active socket, use #pj_activesock_set_iphone_os_bg() instead.
 * By default, this setting is enabled.
 *
 * This API is only available if PJ_IPHONE_OS_HAS_MULTITASKING_SUPPORT
 * is set to non-zero.
 *
 * @param val	    The value of global background mode setting.
 *
 */
PJ_DECL(void) pj_activesock_enable_iphone_os_bg(pj_bool_t val);
#endif

/**
 * 设置用户数据，应用程序可以检查回调中的数据，并将其与更高级别的处理相关联。
 *
 * @param asock	    The active socket.
 * @param user_data 活动 socket 的用户数据
 *
 * @return	    操作成功返回 PJ_SUCCESS，操作失败返回相关的错误码
 */
PJ_DECL(pj_status_t) pj_activesock_set_user_data(pj_activesock_t *asock,
						 void *user_data);

/**
 * 获取活动 socket 的用户数据
 *
 * @param asock	    活动 socket
 *
 * @return	    用户数据
 */
PJ_DECL(void*) pj_activesock_get_user_data(pj_activesock_t *asock);


/**
 * 在此活动套接字上启动读取操作。此函数将创建 async_cnt数量的缓冲区（pj_activesock_create() 函数中给出了
 * async_cnt参数），其中每个缓冲区的长度为 buff_size。缓冲区是从指定的池中分配。一旦创建了缓冲区，它就会向套接
 * 字发出 async_cnt 数量的异步 recv() 操作，并返回给调用者。套接字上的传入数据将通过 on_data_read()回调返回给应用程序。
 *
 * 应用程序只需调用此函数一次即可启动读取操作。当 on_data_read() 回调返回非零时，活动套接字将自动执行进一步的读取操作。
 *
 * @param asock	    The active socket.
 * @param pool	    接收数据分配存储的缓冲区
 * @param buff_size 每个缓存的大小
 * @param flags	    pj_ioqueue_recv() 的标识
 *
 * @return	    操作成功返回 PJ_SUCCESS，否则返回相关的错误码
 */
PJ_DECL(pj_status_t) pj_activesock_start_read(pj_activesock_t *asock,
					      pj_pool_t *pool,
					      unsigned buff_size,
					      pj_uint32_t flags);

/**
 * 与 pj_activesock_start_read() 相同，只是应用程序为读取操作提供缓冲区，以便活动套接字不必分配缓冲区。
 *
 * @param asock	    The active socket.
 * @param pool	    接收数据分配存储的缓冲区
 * @param buff_size 每个缓存的大小
 * @param readbuf   数据包缓存数组，每个缓存都是 buff_size 大小
 * @param flags	    pj_ioqueue_recv() 的标识
 *
 * @return	    操作成功返回 PJ_SUCCESS，操作失败返回相应错误码
 */
PJ_DECL(pj_status_t) pj_activesock_start_read2(pj_activesock_t *asock,
					       pj_pool_t *pool,
					       unsigned buff_size,
					       void *readbuf[],
					       pj_uint32_t flags);

/**
 * 与 pj_activesock_start_read() 相同，只是此函数仅用于UDP，它将触发 on_data_recvfrom() 回调。
 *
 * @param asock	    The active socket.
 * @param pool	    接收数据分配存储的缓冲区
 * @param buff_size 每个缓存的大小
 * @param flags	    pj_ioqueue_recvfrom() 的标识
 *
 * @return	    操作成功返回 PJ_SUCCESS，否则返回相关的错误码
 */
PJ_DECL(pj_status_t) pj_activesock_start_recvfrom(pj_activesock_t *asock,
						  pj_pool_t *pool,
						  unsigned buff_size,
						  pj_uint32_t flags);

/**
 * 与 pj_activesock_start_recvfrom() 相同，只是 recvfrom（）操作从参数中获取缓冲区，而不是创建新的缓冲区。
 *
 * @param asock	    The active socket.
 * @param pool	    接收数据分配存储的缓冲区
 * @param buff_size 每个缓存的大小
 * @param readbuf   数据包缓存数组，每个缓存都是 buff_size 大小
 * @param flags	    pj_ioqueue_recvfrom() 的标识
 *
 * @return	    PJ_SUCCESS if the operation has been successful,
 *		    or the appropriate error code on failure.
 */
PJ_DECL(pj_status_t) pj_activesock_start_recvfrom2(pj_activesock_t *asock,
						   pj_pool_t *pool,
						   unsigned buff_size,
						   void *readbuf[],
						   pj_uint32_t flags);

/**
 * 发送数据
 *
 * @param asock	    The active socket.
 * @param send_key  发送数据的 key，如果应用程序希望提交多个挂起的发送操作，并且希望跟踪在 on_data_sent()回调中发送的确切数据，则此操作键非常有用。
 * @param data	    要发送的数据。在发送数据之前，此数据必须保持有效
 * @param size	    发送数据的大小
 * @param flags	    pj_ioqueue_send() 的标识
 *
 *
 * @return			如果数据已立即发送，则返回 PJ_SUCCESS；如果数据无法立即发送，则返回 PJ_EPENDING，在这种情况下，
 * 将在实际发送数据时调用 on_data_sent() 回调。任何其他返回值都表示错误情况。
 */
PJ_DECL(pj_status_t) pj_activesock_send(pj_activesock_t *asock,
					pj_ioqueue_op_key_t *send_key,
					const void *data,
					pj_ssize_t *size,
					unsigned flags);

/**
 * 发送 数据报数据
 *
 * @param asock	    The active socket.
 * @param send_key  发送数据的 key，如果应用程序希望提交多个挂起的发送操作，并且希望跟踪在 on_data_sent()回调中发送的确切数据，
 * 					则此 key 非常有用
 * @param data	    要发送的数据。在发送数据之前，此数据必须保持有效
 * @param size	    发送数据的大小
 * @param flags	    pj_ioqueue_send() 的标识
 * @param addr	    目的地址
 * @param addr_len  地址的长度
 *
 * @return			如果数据已立即发送，则返回 PJ_SUCCESS；如果数据无法立即发送，则返回PJ_EPENDING。在这种情况下，将在实际发送
 * 数据时调用 on_data_sent() 回调。任何其他返回值都表示错误情况。
 */
PJ_DECL(pj_status_t) pj_activesock_sendto(pj_activesock_t *asock,
					  pj_ioqueue_op_key_t *send_key,
					  const void *data,
					  pj_ssize_t *size,
					  unsigned flags,
					  const pj_sockaddr_t *addr,
					  int addr_len);

#if PJ_HAS_TCP
/**
 * 在此活动套接字上启动异步socket accept（）操作。应用程序必须在调用此函数之前绑定套接字。此函数将向套接字发出
 * async_cnt 个异步 accept() 操作，并返回给调用者。套接字上的传入连接将通过 on_accept_complete() 回调返回给应用程序。
 *
 * 应用程序只需调用此函数一次即可启动accept（）操作。当 on_accept_complete() 回调返回非零时，活动套接字将自动执行进一步
 * 的 accept() 操作。
 *
 * @param asock	    活动的 socket
 * @param pool	    分配内部数据的缓存池
 *
 * @return	    操作成功返回 PJ_SUCCESS，操作失败返回错误码
 */
PJ_DECL(pj_status_t) pj_activesock_start_accept(pj_activesock_t *asock,
						pj_pool_t *pool);

/**
 * 为此套接字启动异步socket connect（）操作。一旦连接完成（无论是否成功），将调用 on_connect_complete() 回调。
 *
 * @param asock	    The active socket.
 * @param pool	    分配内部数据的缓存池
 * @param remaddr   远端地址
 * @param addr_len  远端地址的长度
 *
 * @return		如果可以立即建立连接，则返回PJ_SUCCESS；如果无法立即建立连接，则返回PJ_EPENDING。在这种情况下，
 * 连接完成时将调用 on_connect_complete() 回调。任何其他返回值都表示错误情况。
 */
PJ_DECL(pj_status_t) pj_activesock_start_connect(pj_activesock_t *asock,
						 pj_pool_t *pool,
						 const pj_sockaddr_t *remaddr,
						 int addr_len);


#endif	/* PJ_HAS_TCP */

/**
 * @}
 */

PJ_END_DECL

#endif	/* __PJ_ASYNCSOCK_H__ */

