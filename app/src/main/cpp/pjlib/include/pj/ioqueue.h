/* $Id: ioqueue.h 5194 2015-11-06 04:18:46Z nanang $
 */
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
 * Note that when key concurrency is enabled (i.e. parallel callback calls on
 * the same key is allowed; this is the default setting), the ioqueue will only
 * perform simultaneous callback executions on the same key when the key has
 * invoked multiple pending operations. This could be done for example by
 * calling #pj_ioqueue_recvfrom() more than once on the same key, each with
 * the same key but different operation key (pj_ioqueue_op_key_t). With this
 * scenario, when multiple packets arrive on the key at the same time, more
 * than one threads may execute the callback simultaneously, each with the
 * same key but different operation key.
 *
 * When there is only one pending operation on the key (e.g. there is only one
 * #pj_ioqueue_recvfrom() invoked on the key), then events occuring to the
 * same key will be queued by the ioqueue, thus no simultaneous callback calls
 * will be performed.
 *
 * \subsection pj_ioq_allow_concur Concurrency is Enabled (Default Value)
 *
 * The default setting for the ioqueue is to allow multiple threads to
 * execute callbacks for the same handle/key. This setting is selected to
 * promote good performance and scalability for application.
 *
 * However this setting has a major drawback with regard to synchronization,
 * and application MUST carefully follow the following guidelines to ensure 
 * that parallel access to the key does not cause problems:
 *
 *  - Always note that callback may be called simultaneously for the same
 *    key.
 *  - <b>Care must be taken when unregistering a key</b> from the
 *    ioqueue. Application must take care that when one thread is issuing
 *    an unregistration, other thread is not simultaneously invoking the
 *    callback <b>to the same key</b>.
 *\n
 *    This happens because the ioqueue functions are working with a pointer
 *    to the key, and there is a possible race condition where the pointer
 *    has been rendered invalid by other threads before the ioqueue has a
 *    chance to acquire mutex on it.
 *
 * \subsection pj_ioq_disallow_concur Concurrency is Disabled
 *
 * Alternatively, application may disable key concurrency to make 
 * synchronization easier. As noted above, there are three ways to control
 * key concurrency setting:
 *  - by controlling on per handle/key basis, with #pj_ioqueue_set_concurrency().
 *  - by changing default key concurrency setting on the ioqueue, with
 *    #pj_ioqueue_set_default_concurrency().
 *  - by changing the default concurrency on compile time, by declaring
 *    PJ_IOQUEUE_DEFAULT_ALLOW_CONCURRENCY macro to zero in your config_site.h
 *
 * \section pj_ioqeuue_examples_sec Examples
 *
 * For some examples on how to use the I/O Queue, please see:
 *
 *  - \ref page_pjlib_ioqueue_tcp_test
 *  - \ref page_pjlib_ioqueue_udp_test
 *  - \ref page_pjlib_ioqueue_perf_test
 */


/**
 * This structure describes operation specific key to be submitted to
 * I/O Queue when performing the asynchronous operation. This key will
 * be returned to the application when completion callback is called.
 *
 * Application normally wants to attach it's specific data in the
 * \c user_data field so that it can keep track of which operation has
 * completed when the callback is called. Alternatively, application can
 * also extend this struct to include its data, because the pointer that
 * is returned in the completion callback will be exactly the same as
 * the pointer supplied when the asynchronous function is called.
 */
typedef struct pj_ioqueue_op_key_t
{ 
    void *internal__[32];           /**< Internal I/O Queue data.   */
    void *activesock_data;	    /**< Active socket data.	    */
    void *user_data;                /**< Application data.          */
} pj_ioqueue_op_key_t;

/**
 * This structure describes the callbacks to be called when I/O operation
 * completes.
 */
typedef struct pj_ioqueue_callback
{
    /**
     * This callback is called when #pj_ioqueue_recv or #pj_ioqueue_recvfrom
     * completes.
     *
     * @param key	    The key.
     * @param op_key        Operation key.
     * @param bytes_read    >= 0 to indicate the amount of data read, 
     *                      otherwise negative value containing the error
     *                      code. To obtain the pj_status_t error code, use
     *                      (pj_status_t code = -bytes_read).
     */
    void (*on_read_complete)(pj_ioqueue_key_t *key, 
                             pj_ioqueue_op_key_t *op_key, 
                             pj_ssize_t bytes_read);

    /**
     * This callback is called when #pj_ioqueue_send or #pj_ioqueue_sendto
     * completes.
     *
     * @param key	    The key.
     * @param op_key        Operation key.
     * @param bytes_sent    >= 0 to indicate the amount of data written, 
     *                      otherwise negative value containing the error
     *                      code. To obtain the pj_status_t error code, use
     *                      (pj_status_t code = -bytes_sent).
     */
    void (*on_write_complete)(pj_ioqueue_key_t *key, 
                              pj_ioqueue_op_key_t *op_key, 
                              pj_ssize_t bytes_sent);

    /**
     * This callback is called when #pj_ioqueue_accept completes.
     *
     * @param key	    The key.
     * @param op_key        Operation key.
     * @param sock          Newly connected socket.
     * @param status	    Zero if the operation completes successfully.
     */
    void (*on_accept_complete)(pj_ioqueue_key_t *key, 
                               pj_ioqueue_op_key_t *op_key, 
                               pj_sock_t sock, 
                               pj_status_t status);

    /**
     * This callback is called when #pj_ioqueue_connect completes.
     *
     * @param key	    The key.
     * @param status	    PJ_SUCCESS if the operation completes successfully.
     */
    void (*on_connect_complete)(pj_ioqueue_key_t *key, 
                                pj_status_t status);
} pj_ioqueue_callback;


/**
 * Types of pending I/O Queue operation. This enumeration is only used
 * internally within the ioqueue.
 */
typedef enum pj_ioqueue_operation_e
{
    PJ_IOQUEUE_OP_NONE		= 0,	/**< No operation.          */
    PJ_IOQUEUE_OP_READ		= 1,	/**< read() operation.      */
    PJ_IOQUEUE_OP_RECV          = 2,    /**< recv() operation.      */
    PJ_IOQUEUE_OP_RECV_FROM	= 4,	/**< recvfrom() operation.  */
    PJ_IOQUEUE_OP_WRITE		= 8,	/**< write() operation.     */
    PJ_IOQUEUE_OP_SEND          = 16,   /**< send() operation.      */
    PJ_IOQUEUE_OP_SEND_TO	= 32,	/**< sendto() operation.    */
#if defined(PJ_HAS_TCP) && PJ_HAS_TCP != 0
    PJ_IOQUEUE_OP_ACCEPT	= 64,	/**< accept() operation.    */
    PJ_IOQUEUE_OP_CONNECT	= 128	/**< connect() operation.   */
#endif	/* PJ_HAS_TCP */
} pj_ioqueue_operation_e;


/**
 * This macro specifies the maximum number of events that can be
 * processed by the ioqueue on a single poll cycle, on implementation
 * that supports it. The value is only meaningfull when specified
 * during PJLIB build.
 */
#ifndef PJ_IOQUEUE_MAX_EVENTS_IN_SINGLE_POLL
#   define PJ_IOQUEUE_MAX_EVENTS_IN_SINGLE_POLL     (16)
#endif


/**
 * This macro specifies the maximum event candidates collected by each
 * polling thread to be able to reach maximum number of processed events
 * (i.e: PJ_IOQUEUE_MAX_EVENTS_IN_SINGLE_POLL) in each poll cycle.
 * An event candidate will be dispatched to application as event unless
 * it is already being dispatched by other polling thread. So in order to
 * anticipate such race condition, each poll operation should collects its
 * event candidates more than PJ_IOQUEUE_MAX_EVENTS_IN_SINGLE_POLL, the
 * recommended value is (PJ_IOQUEUE_MAX_EVENTS_IN_SINGLE_POLL *
 * number of polling threads).
 *
 * The value is only meaningfull when specified during PJLIB build and
 * is only effective on multiple polling threads environment.
 */
#if !defined(PJ_IOQUEUE_MAX_CAND_EVENTS) || \
    PJ_IOQUEUE_MAX_CAND_EVENTS < PJ_IOQUEUE_MAX_EVENTS_IN_SINGLE_POLL
#   undef  PJ_IOQUEUE_MAX_CAND_EVENTS
#   define PJ_IOQUEUE_MAX_CAND_EVENTS	PJ_IOQUEUE_MAX_EVENTS_IN_SINGLE_POLL
#endif


/**
 * When this flag is specified in ioqueue's recv() or send() operations,
 * the ioqueue will always mark the operation as asynchronous.
 */
#define PJ_IOQUEUE_ALWAYS_ASYNC	    ((pj_uint32_t)1 << (pj_uint32_t)31)

/**
 * Return the name of the ioqueue implementation.
 *
 * @return		Implementation name.
 */
PJ_DECL(const char*) pj_ioqueue_name(void);


/**
 * Create a new I/O Queue framework.
 *
 * @param pool		The pool to allocate the I/O queue structure. 
 * @param max_fd	The maximum number of handles to be supported, which 
 *			should not exceed PJ_IOQUEUE_MAX_HANDLES.
 * @param ioqueue	Pointer to hold the newly created I/O Queue.
 *
 * @return		PJ_SUCCESS on success.
 */
PJ_DECL(pj_status_t) pj_ioqueue_create( pj_pool_t *pool, 
					pj_size_t max_fd,
					pj_ioqueue_t **ioqueue);

/**
 * Destroy the I/O queue.
 *
 * @param ioque	        The I/O Queue to be destroyed.
 *
 * @return              PJ_SUCCESS if success.
 */
PJ_DECL(pj_status_t) pj_ioqueue_destroy( pj_ioqueue_t *ioque );

/**
 * Set the lock object to be used by the I/O Queue. This function can only
 * be called right after the I/O queue is created, before any handle is
 * registered to the I/O queue.
 *
 * Initially the I/O queue is created with non-recursive mutex protection. 
 * Applications can supply alternative lock to be used by calling this 
 * function.
 *
 * @param ioque         The ioqueue instance.
 * @param lock          The lock to be used by the ioqueue.
 * @param auto_delete   In non-zero, the lock will be deleted by the ioqueue.
 *
 * @return              PJ_SUCCESS or the appropriate error code.
 */
PJ_DECL(pj_status_t) pj_ioqueue_set_lock( pj_ioqueue_t *ioque, 
					  pj_lock_t *lock,
					  pj_bool_t auto_delete );

/**
 * Set default concurrency policy for this ioqueue. If this function is not
 * called, the default concurrency policy for the ioqueue is controlled by 
 * compile time setting PJ_IOQUEUE_DEFAULT_ALLOW_CONCURRENCY.
 *
 * Note that changing the concurrency setting to the ioqueue will only affect
 * subsequent key registrations. To modify the concurrency setting for
 * individual key, use #pj_ioqueue_set_concurrency().
 *
 * @param ioqueue	The ioqueue instance.
 * @param allow		Non-zero to allow concurrent callback calls, or
 *			PJ_FALSE to disallow it.
 *
 * @return		PJ_SUCCESS on success or the appropriate error code.
 */
PJ_DECL(pj_status_t) pj_ioqueue_set_default_concurrency(pj_ioqueue_t *ioqueue,
							pj_bool_t allow);

/**
 * Register a socket to the I/O queue framework. 
 * When a socket is registered to the IOQueue, it may be modified to use
 * non-blocking IO. If it is modified, there is no guarantee that this 
 * modification will be restored after the socket is unregistered.
 *
 * @param pool	    To allocate the resource for the specified handle, 
 *		    which must be valid until the handle/key is unregistered 
 *		    from I/O Queue.
 * @param ioque	    The I/O Queue.
 * @param sock	    The socket.
 * @param user_data User data to be associated with the key, which can be
 *		    retrieved later.
 * @param cb	    Callback to be called when I/O operation completes. 
 * @param key       Pointer to receive the key to be associated with this
 *                  socket. Subsequent I/O queue operation will need this
 *                  key.
 *
 * @return	    PJ_SUCCESS on success, or the error code.
 */
PJ_DECL(pj_status_t) pj_ioqueue_register_sock( pj_pool_t *pool,
					       pj_ioqueue_t *ioque,
					       pj_sock_t sock,
					       void *user_data,
					       const pj_ioqueue_callback *cb,
                                               pj_ioqueue_key_t **key );

/**
 * Variant of pj_ioqueue_register_sock() with additional group lock parameter.
 * If group lock is set for the key, the key will add the reference counter
 * when the socket is registered and decrease it when it is destroyed.
 */
PJ_DECL(pj_status_t) pj_ioqueue_register_sock2(pj_pool_t *pool,
					       pj_ioqueue_t *ioque,
					       pj_sock_t sock,
					       pj_grp_lock_t *grp_lock,
					       void *user_data,
					       const pj_ioqueue_callback *cb,
                                               pj_ioqueue_key_t **key );

/**
 * Unregister from the I/O Queue framework. Caller must make sure that
 * the key doesn't have any pending operations before calling this function,
 * by calling #pj_ioqueue_is_pending() for all previously submitted
 * operations except asynchronous connect, and if necessary call
 * #pj_ioqueue_post_completion() to cancel the pending operations.
 *
 * Note that asynchronous connect operation will automatically be 
 * cancelled during the unregistration.
 *
 * Also note that when I/O Completion Port backend is used, application
 * MUST close the handle immediately after unregistering the key. This is
 * because there is no unregistering API for IOCP. The only way to
 * unregister the handle from IOCP is to close the handle.
 *
 * @param key	    The key that was previously obtained from registration.
 *
 * @return          PJ_SUCCESS on success or the error code.
 *
 * @see pj_ioqueue_is_pending
 */
PJ_DECL(pj_status_t) pj_ioqueue_unregister( pj_ioqueue_key_t *key );


/**
 * Get user data associated with an ioqueue key.
 *
 * @param key	    The key that was previously obtained from registration.
 *
 * @return          The user data associated with the descriptor, or NULL 
 *                  on error or if no data is associated with the key during
 *                  registration.
 */
PJ_DECL(void*) pj_ioqueue_get_user_data( pj_ioqueue_key_t *key );

/**
 * Set or change the user data to be associated with the file descriptor or
 * handle or socket descriptor.
 *
 * @param key	    The key that was previously obtained from registration.
 * @param user_data User data to be associated with the descriptor.
 * @param old_data  Optional parameter to retrieve the old user data.
 *
 * @return          PJ_SUCCESS on success or the error code.
 */
PJ_DECL(pj_status_t) pj_ioqueue_set_user_data( pj_ioqueue_key_t *key,
                                               void *user_data,
                                               void **old_data);

/**
 * Configure whether the ioqueue is allowed to call the key's callback
 * concurrently/in parallel. The default concurrency setting for the key
 * is controlled by ioqueue's default concurrency value, which can be
 * changed by calling #pj_ioqueue_set_default_concurrency().
 *
 * If concurrency is allowed for the key, it means that if there are more
 * than one pending operations complete simultaneously, more than one
 * threads may call the key's  callback at the same time. This generally
 * would promote good scalability for application, at the expense of more
 * complexity to manage the concurrent accesses in application's code.
 *
 * Alternatively application may disable the concurrent access by
 * setting the \a allow flag to false. With concurrency disabled, only
 * one thread can call the key's callback at one time.
 *
 * @param key	    The key that was previously obtained from registration.
 * @param allow	    Set this to non-zero to allow concurrent callback calls
 *		    and zero (PJ_FALSE) to disallow it.
 *
 * @return	    PJ_SUCCESS on success or the appropriate error code.
 */
PJ_DECL(pj_status_t) pj_ioqueue_set_concurrency(pj_ioqueue_key_t *key,
						pj_bool_t allow);

/**
 * Acquire the key's mutex. When the key's concurrency is disabled, 
 * application may call this function to synchronize its operation
 * with the key's callback (i.e. this function will block until the
 * key's callback returns).
 *
 * @param key	    The key that was previously obtained from registration.
 *
 * @return	    PJ_SUCCESS on success or the appropriate error code.
 */
PJ_DECL(pj_status_t) pj_ioqueue_lock_key(pj_ioqueue_key_t *key);

/**
 * Try to acquire the key's mutex. When the key's concurrency is disabled, 
 * application may call this function to synchronize its operation
 * with the key's callback.
 *
 * @param key	    The key that was previously obtained from registration.
 *
 * @return	    PJ_SUCCESS on success or the appropriate error code.
 */
PJ_DECL(pj_status_t) pj_ioqueue_trylock_key(pj_ioqueue_key_t *key);

/**
 * Release the lock previously acquired with pj_ioqueue_lock_key().
 *
 * @param key	    The key that was previously obtained from registration.
 *
 * @return	    PJ_SUCCESS on success or the appropriate error code.
 */
PJ_DECL(pj_status_t) pj_ioqueue_unlock_key(pj_ioqueue_key_t *key);

/**
 * Initialize operation key.
 *
 * @param op_key    The operation key to be initialied.
 * @param size	    The size of the operation key.
 */
PJ_DECL(void) pj_ioqueue_op_key_init( pj_ioqueue_op_key_t *op_key,
				      pj_size_t size );

/**
 * Check if operation is pending on the specified operation key.
 * The \c op_key must have been initialized with #pj_ioqueue_op_key_init() 
 * or submitted as pending operation before, or otherwise the result 
 * is undefined.
 *
 * @param key       The key.
 * @param op_key    The operation key, previously submitted to any of
 *                  the I/O functions and has returned PJ_EPENDING.
 *
 * @return          Non-zero if operation is still pending.
 */
PJ_DECL(pj_bool_t) pj_ioqueue_is_pending( pj_ioqueue_key_t *key,
                                          pj_ioqueue_op_key_t *op_key );


/**
 * Post completion status to the specified operation key and call the
 * appropriate callback. When the callback is called, the number of bytes 
 * received in read/write callback or the status in accept/connect callback
 * will be set from the \c bytes_status parameter.
 *
 * @param key           The key.
 * @param op_key        Pending operation key.
 * @param bytes_status  Number of bytes or status to be set. A good value
 *                      to put here is -PJ_ECANCELLED.
 *
 * @return              PJ_SUCCESS if completion status has been successfully
 *                      sent.
 */
PJ_DECL(pj_status_t) pj_ioqueue_post_completion( pj_ioqueue_key_t *key,
                                                 pj_ioqueue_op_key_t *op_key,
                                                 pj_ssize_t bytes_status );



#if defined(PJ_HAS_TCP) && PJ_HAS_TCP != 0
/**
 * Instruct I/O Queue to accept incoming connection on the specified 
 * listening socket. This function will return immediately (i.e. non-blocking)
 * regardless whether a connection is immediately available. If the function
 * can't complete immediately, the caller will be notified about the incoming
 * connection when it calls pj_ioqueue_poll(). If a new connection is
 * immediately available, the function returns PJ_SUCCESS with the new
 * connection; in this case, the callback WILL NOT be called.
 *
 * @param key	    The key which registered to the server socket.
 * @param op_key    An operation specific key to be associated with the
 *                  pending operation, so that application can keep track of
 *                  which operation has been completed when the callback is
 *                  called.
 * @param new_sock  Argument which contain pointer to receive the new socket
 *                  for the incoming connection.
 * @param local	    Optional argument which contain pointer to variable to 
 *                  receive local address.
 * @param remote    Optional argument which contain pointer to variable to 
 *                  receive the remote address.
 * @param addrlen   On input, contains the length of the buffer for the
 *		    address, and on output, contains the actual length of the
 *		    address. This argument is optional.
 * @return
 *  - PJ_SUCCESS    When connection is available immediately, and the 
 *                  parameters will be updated to contain information about 
 *                  the new connection. In this case, a completion callback
 *                  WILL NOT be called.
 *  - PJ_EPENDING   If no connection is available immediately. When a new
 *                  connection arrives, the callback will be called.
 *  - non-zero      which indicates the appropriate error code.
 */
PJ_DECL(pj_status_t) pj_ioqueue_accept( pj_ioqueue_key_t *key,
                                        pj_ioqueue_op_key_t *op_key,
					pj_sock_t *new_sock,
					pj_sockaddr_t *local,
					pj_sockaddr_t *remote,
					int *addrlen );

/**
 * Initiate non-blocking socket connect. If the socket can NOT be connected
 * immediately, asynchronous connect() will be scheduled and caller will be
 * notified via completion callback when it calls pj_ioqueue_poll(). If
 * socket is connected immediately, the function returns PJ_SUCCESS and
 * completion callback WILL NOT be called.
 *
 * @param key	    The key associated with TCP socket
 * @param addr	    The remote address.
 * @param addrlen   The remote address length.
 *
 * @return
 *  - PJ_SUCCESS    If socket is connected immediately. In this case, the
 *                  completion callback WILL NOT be called.
 *  - PJ_EPENDING   If operation is queued, or 
 *  - non-zero      Indicates the error code.
 */
PJ_DECL(pj_status_t) pj_ioqueue_connect( pj_ioqueue_key_t *key,
					 const pj_sockaddr_t *addr,
					 int addrlen );

#endif	/* PJ_HAS_TCP */

/**
 * Poll the I/O Queue for completed events.
 *
 * Note: polling the ioqueue is not necessary in Symbian. Please see
 * @ref PJ_SYMBIAN_OS for more info.
 *
 * @param ioque		the I/O Queue.
 * @param timeout	polling timeout, or NULL if the thread wishes to wait
 *			indefinetely for the event.
 *
 * @return 
 *  - zero if timed out (no event).
 *  - (<0) if error occured during polling. Callback will NOT be called.
 *  - (>1) to indicate numbers of events. Callbacks have been called.
 */
PJ_DECL(int) pj_ioqueue_poll( pj_ioqueue_t *ioque,
			      const pj_time_val *timeout);


/**
 * Instruct the I/O Queue to read from the specified handle. This function
 * returns immediately (i.e. non-blocking) regardless whether some data has 
 * been transferred. If the operation can't complete immediately, caller will 
 * be notified about the completion when it calls pj_ioqueue_poll(). If data
 * is immediately available, the function will return PJ_SUCCESS and the
 * callback WILL NOT be called.
 *
 * @param key	    The key that uniquely identifies the handle.
 * @param op_key    An operation specific key to be associated with the
 *                  pending operation, so that application can keep track of
 *                  which operation has been completed when the callback is
 *                  called. Caller must make sure that this key remains 
 *                  valid until the function completes.
 * @param buffer    The buffer to hold the read data. The caller MUST make sure
 *		    that this buffer remain valid until the framework completes
 *		    reading the handle.
 * @param length    On input, it specifies the size of the buffer. If data is
 *                  available to be read immediately, the function returns
 *                  PJ_SUCCESS and this argument will be filled with the
 *                  amount of data read. If the function is pending, caller
 *                  will be notified about the amount of data read in the
 *                  callback. This parameter can point to local variable in
 *                  caller's stack and doesn't have to remain valid for the
 *                  duration of pending operation.
 * @param flags     Recv flag. If flags has PJ_IOQUEUE_ALWAYS_ASYNC then
 *		    the function will never return PJ_SUCCESS.
 *
 * @return
 *  - PJ_SUCCESS    If immediate data has been received in the buffer. In this
 *                  case, the callback WILL NOT be called.
 *  - PJ_EPENDING   If the operation has been queued, and the callback will be
 *                  called when data has been received.
 *  - non-zero      The return value indicates the error code.
 */
PJ_DECL(pj_status_t) pj_ioqueue_recv( pj_ioqueue_key_t *key,
                                      pj_ioqueue_op_key_t *op_key,
				      void *buffer,
				      pj_ssize_t *length,
				      pj_uint32_t flags );

/**
 * This function behaves similarly as #pj_ioqueue_recv(), except that it is
 * normally called for socket, and the remote address will also be returned
 * along with the data. Caller MUST make sure that both buffer and addr
 * remain valid until the framework completes reading the data.
 *
 * @param key	    The key that uniquely identifies the handle.
 * @param op_key    An operation specific key to be associated with the
 *                  pending operation, so that application can keep track of
 *                  which operation has been completed when the callback is
 *                  called.
 * @param buffer    The buffer to hold the read data. The caller MUST make sure
 *		    that this buffer remain valid until the framework completes
 *		    reading the handle.
 * @param length    On input, it specifies the size of the buffer. If data is
 *                  available to be read immediately, the function returns
 *                  PJ_SUCCESS and this argument will be filled with the
 *                  amount of data read. If the function is pending, caller
 *                  will be notified about the amount of data read in the
 *                  callback. This parameter can point to local variable in
 *                  caller's stack and doesn't have to remain valid for the
 *                  duration of pending operation.
 * @param flags     Recv flag. If flags has PJ_IOQUEUE_ALWAYS_ASYNC then
 *		    the function will never return PJ_SUCCESS.
 * @param addr      Optional Pointer to buffer to receive the address.
 * @param addrlen   On input, specifies the length of the address buffer.
 *                  On output, it will be filled with the actual length of
 *                  the address. This argument can be NULL if \c addr is not
 *                  specified.
 *
 * @return
 *  - PJ_SUCCESS    If immediate data has been received. In this case, the 
 *		    callback must have been called before this function 
 *		    returns, and no pending operation is scheduled.
 *  - PJ_EPENDING   If the operation has been queued.
 *  - non-zero      The return value indicates the error code.
 */
PJ_DECL(pj_status_t) pj_ioqueue_recvfrom( pj_ioqueue_key_t *key,
                                          pj_ioqueue_op_key_t *op_key,
					  void *buffer,
					  pj_ssize_t *length,
                                          pj_uint32_t flags,
					  pj_sockaddr_t *addr,
					  int *addrlen);

/**
 * Instruct the I/O Queue to write to the handle. This function will return
 * immediately (i.e. non-blocking) regardless whether some data has been 
 * transferred. If the function can't complete immediately, the caller will
 * be notified about the completion when it calls pj_ioqueue_poll(). If 
 * operation completes immediately and data has been transferred, the function
 * returns PJ_SUCCESS and the callback will NOT be called.
 *
 * @param key	    The key that identifies the handle.
 * @param op_key    An operation specific key to be associated with the
 *                  pending operation, so that application can keep track of
 *                  which operation has been completed when the callback is
 *                  called.
 * @param data	    The data to send. Caller MUST make sure that this buffer 
 *		    remains valid until the write operation completes.
 * @param length    On input, it specifies the length of data to send. When
 *                  data was sent immediately, this function returns PJ_SUCCESS
 *                  and this parameter contains the length of data sent. If
 *                  data can not be sent immediately, an asynchronous operation
 *                  is scheduled and caller will be notified via callback the
 *                  number of bytes sent. This parameter can point to local 
 *                  variable on caller's stack and doesn't have to remain 
 *                  valid until the operation has completed.
 * @param flags     Send flags. If flags has PJ_IOQUEUE_ALWAYS_ASYNC then
 *		    the function will never return PJ_SUCCESS.
 *
 * @return
 *  - PJ_SUCCESS    If data was immediately transferred. In this case, no
 *                  pending operation has been scheduled and the callback
 *                  WILL NOT be called.
 *  - PJ_EPENDING   If the operation has been queued. Once data base been
 *                  transferred, the callback will be called.
 *  - non-zero      The return value indicates the error code.
 */
PJ_DECL(pj_status_t) pj_ioqueue_send( pj_ioqueue_key_t *key,
                                      pj_ioqueue_op_key_t *op_key,
				      const void *data,
				      pj_ssize_t *length,
				      pj_uint32_t flags );


/**
 * Instruct the I/O Queue to write to the handle. This function will return
 * immediately (i.e. non-blocking) regardless whether some data has been 
 * transferred. If the function can't complete immediately, the caller will
 * be notified about the completion when it calls pj_ioqueue_poll(). If 
 * operation completes immediately and data has been transferred, the function
 * returns PJ_SUCCESS and the callback will NOT be called.
 *
 * @param key	    the key that identifies the handle.
 * @param op_key    An operation specific key to be associated with the
 *                  pending operation, so that application can keep track of
 *                  which operation has been completed when the callback is
 *                  called.
 * @param data	    the data to send. Caller MUST make sure that this buffer 
 *		    remains valid until the write operation completes.
 * @param length    On input, it specifies the length of data to send. When
 *                  data was sent immediately, this function returns PJ_SUCCESS
 *                  and this parameter contains the length of data sent. If
 *                  data can not be sent immediately, an asynchronous operation
 *                  is scheduled and caller will be notified via callback the
 *                  number of bytes sent. This parameter can point to local 
 *                  variable on caller's stack and doesn't have to remain 
 *                  valid until the operation has completed.
 * @param flags     send flags. If flags has PJ_IOQUEUE_ALWAYS_ASYNC then
 *		    the function will never return PJ_SUCCESS.
 * @param addr      Optional remote address.
 * @param addrlen   Remote address length, \c addr is specified.
 *
 * @return
 *  - PJ_SUCCESS    If data was immediately written.
 *  - PJ_EPENDING   If the operation has been queued.
 *  - non-zero      The return value indicates the error code.
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

