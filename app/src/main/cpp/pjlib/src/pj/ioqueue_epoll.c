/**
 * 已完成：
 *      1. 创建 ioqueue  /销毁 ioqueue
 *      2. 注册 socket 句柄到 ioqueue    / socket句柄从 ioqueue 中注销
 *      3. 从队列中移除指定 key,type 的事件      （一般poll dispatch 分发后）
 *      4. 添加指定 key,type 的事件到 ioqueue
 *      5. ioqueue_poll() 轮询事件  （epoll_wait()）
 *
 *      epoll_create()
 *      epoll_ctl()
 *      epoll_wait()
 *
 * 这是在Linux用户模式和内核模式下使用/dev/epoll API实现IO Queue框架
 */

#include <pj/ioqueue.h>
#include <pj/os.h>
#include <pj/lock.h>
#include <pj/log.h>
#include <pj/list.h>
#include <pj/pool.h>
#include <pj/string.h>
#include <pj/assert.h>
#include <pj/errno.h>
#include <pj/sock.h>
#include <pj/compat/socket.h>
#include <pj/rand.h>

#include <sys/epoll.h>
#include <errno.h>
#include <unistd.h>

#define epoll_data		data.ptr
#define epoll_data_type		void*
#define ioctl_val_type		unsigned long
#define getsockopt_val_ptr	int*
#define os_getsockopt		getsockopt
#define os_ioctl		ioctl
#define os_read			read
#define os_close		close
#define os_epoll_create		epoll_create
#define os_epoll_ctl		epoll_ctl
#define os_epoll_wait		epoll_wait

#define THIS_FILE   "ioq_epoll"

//#define TRACE_(expr) PJ_LOG(3,expr)
#define TRACE_(expr)

/*
 * 包含通用 ioqueue 抽象
 */
#include "ioqueue_common_abs.h"

/*
 * 键的描述
 */
struct pj_ioqueue_key_t
{
    DECLARE_COMMON_KEY
};

struct queue
{
    pj_ioqueue_key_t	    *key;
    enum ioqueue_event_type  event_type;
};

/*
 *  I/O 队列的描述
 */
struct pj_ioqueue_t
{
    DECLARE_COMMON_IOQUEUE

    unsigned		max, count;
    //pj_ioqueue_key_t	hlist;
    pj_ioqueue_key_t	active_list;
    int			epfd;
    //struct epoll_event *events;
    //struct queue       *queue;

#if PJ_IOQUEUE_HAS_SAFE_UNREG
    pj_mutex_t	       *ref_cnt_mutex;
    pj_ioqueue_key_t	closing_list;
    pj_ioqueue_key_t	free_list;
#endif
};

/* 在声明pj_ioqueue_key_t和pj_ioqueue_t之后，包含公共抽象的实现。
 */
#include "ioqueue_common_abs.c"

#if PJ_IOQUEUE_HAS_SAFE_UNREG
/* 再次扫描要放入空闲列表的关闭键 */
static void scan_closing_keys(pj_ioqueue_t *ioqueue);
#endif

/*
 * pj_ioqueue_name()
 */
PJ_DEF(const char*) pj_ioqueue_name(void)
{
    return "epoll";
}

/*
 * pj_ioqueue_create()
 *
 * 创建 select 队列
 */
PJ_DEF(pj_status_t) pj_ioqueue_create( pj_pool_t *pool,
                                       pj_size_t max_fd,
                                       pj_ioqueue_t **p_ioqueue)
{
    pj_ioqueue_t *ioqueue;
    pj_status_t rc;
    pj_lock_t *lock;
    int i;

    /* 检查参数是否有效 */
    PJ_ASSERT_RETURN(pool != NULL && p_ioqueue != NULL &&
                     max_fd > 0, PJ_EINVAL);

    /* 检查 pj_ioqueue_op_key_t 的大小是否足够 */
    PJ_ASSERT_RETURN(sizeof(pj_ioqueue_op_key_t)-sizeof(void*) >=
                     sizeof(union operation_key), PJ_EBUG);

    ioqueue = pj_pool_alloc(pool, sizeof(pj_ioqueue_t));

    ioqueue_init(ioqueue);

    ioqueue->max = max_fd;
    ioqueue->count = 0;
    pj_list_init(&ioqueue->active_list);

#if PJ_IOQUEUE_HAS_SAFE_UNREG
    /* 当使用安全注销（默认设置）时，我们预先创建所有密钥并将它们放入空闲列表中
     */

    /* 互斥量为了保护key的引用计数器，我们不想使用 key 的 Mutex或ioqueue的Mutex，因为这在某些情况下会造成死锁。
     */
    rc = pj_mutex_create_simple(pool, NULL, &ioqueue->ref_cnt_mutex);
    if (rc != PJ_SUCCESS)
	return rc;


    /* 初始化 key列表*/
    pj_list_init(&ioqueue->free_list);
    pj_list_init(&ioqueue->closing_list);


    /* 根据 max_fd 预先创建所有密钥 */
    for ( i=0; i<max_fd; ++i) {
	pj_ioqueue_key_t *key;

	key = PJ_POOL_ALLOC_T(pool, pj_ioqueue_key_t);
	key->ref_count = 0;
	rc = pj_lock_create_recursive_mutex(pool, NULL, &key->lock);
	if (rc != PJ_SUCCESS) {
	    key = ioqueue->free_list.next;
	    while (key != &ioqueue->free_list) {
		pj_lock_destroy(key->lock);
		key = key->next;
	    }
	    pj_mutex_destroy(ioqueue->ref_cnt_mutex);
	    return rc;
	}

	pj_list_push_back(&ioqueue->free_list, key);
    }
#endif

    rc = pj_lock_create_simple_mutex(pool, "ioq%p", &lock);
    if (rc != PJ_SUCCESS)
	return rc;

    rc = pj_ioqueue_set_lock(ioqueue, lock, PJ_TRUE);
    if (rc != PJ_SUCCESS)
        return rc;

    ioqueue->epfd = os_epoll_create(max_fd);
    if (ioqueue->epfd < 0) {
	ioqueue_destroy(ioqueue);
	return PJ_RETURN_OS_ERROR(pj_get_native_os_error());
    }

    /*ioqueue->events = pj_pool_calloc(pool, max_fd, sizeof(struct epoll_event));
    PJ_ASSERT_RETURN(ioqueue->events != NULL, PJ_ENOMEM);

    ioqueue->queue = pj_pool_calloc(pool, max_fd, sizeof(struct queue));
    PJ_ASSERT_RETURN(ioqueue->queue != NULL, PJ_ENOMEM);
   */
    PJ_LOG(4, ("pjlib", "epoll I/O Queue created (%p)", ioqueue));

    *p_ioqueue = ioqueue;
    return PJ_SUCCESS;
}

/*
 * pj_ioqueue_destroy()
 *
 * 销毁 ioqueue.
 */
PJ_DEF(pj_status_t) pj_ioqueue_destroy(pj_ioqueue_t *ioqueue)
{
    pj_ioqueue_key_t *key;

    PJ_ASSERT_RETURN(ioqueue, PJ_EINVAL);
    PJ_ASSERT_RETURN(ioqueue->epfd > 0, PJ_EINVALIDOP);

    pj_lock_acquire(ioqueue->lock);
    os_close(ioqueue->epfd);
    ioqueue->epfd = 0;

#if PJ_IOQUEUE_HAS_SAFE_UNREG
    /* Destroy reference counters */
    key = ioqueue->active_list.next;
    while (key != &ioqueue->active_list) {
	pj_lock_destroy(key->lock);
	key = key->next;
    }

    key = ioqueue->closing_list.next;
    while (key != &ioqueue->closing_list) {
	pj_lock_destroy(key->lock);
	key = key->next;
    }

    key = ioqueue->free_list.next;
    while (key != &ioqueue->free_list) {
	pj_lock_destroy(key->lock);
	key = key->next;
    }

    pj_mutex_destroy(ioqueue->ref_cnt_mutex);
#endif
    return ioqueue_destroy(ioqueue);
}

/*
 * pj_ioqueue_register_sock()
 *
 * 注册一个 socket 到队列
 */
PJ_DEF(pj_status_t) pj_ioqueue_register_sock2(pj_pool_t *pool,
					      pj_ioqueue_t *ioqueue,
					      pj_sock_t sock,
					      pj_grp_lock_t *grp_lock,
					      void *user_data,
					      const pj_ioqueue_callback *cb,
                                              pj_ioqueue_key_t **p_key)
{
    pj_ioqueue_key_t *key = NULL;
    pj_uint32_t value;
    struct epoll_event ev;
    int status;
    pj_status_t rc = PJ_SUCCESS;

    PJ_ASSERT_RETURN(pool && ioqueue && sock != PJ_INVALID_SOCKET &&
                     cb && p_key, PJ_EINVAL);

    pj_lock_acquire(ioqueue->lock);

    if (ioqueue->count >= ioqueue->max) {
        rc = PJ_ETOOMANY;
	TRACE_((THIS_FILE, "pj_ioqueue_register_sock error: too many files"));
	goto on_return;
    }

    /* Set socket 为非阻塞 */
    value = 1;
    if ((rc=os_ioctl(sock, FIONBIO, (ioctl_val_type)&value))) {
	TRACE_((THIS_FILE, "pj_ioqueue_register_sock error: ioctl rc=%d",
                rc));
        rc = pj_get_netos_error();
	goto on_return;
    }

    /* 如果使用了安全注销（PJ_IOQUEUE_HAS_SAFE_UNREG），则从空闲列表中获取密钥。否则，分配一个新的。
     */
#if PJ_IOQUEUE_HAS_SAFE_UNREG

    /* 先扫描closing_keys，让它们返回到free_list */
    scan_closing_keys(ioqueue);

    pj_assert(!pj_list_empty(&ioqueue->free_list));
    if (pj_list_empty(&ioqueue->free_list)) {
	rc = PJ_ETOOMANY;
	goto on_return;
    }

    key = ioqueue->free_list.next;
    pj_list_erase(key);
#else
    /* 创建 key */
    key = (pj_ioqueue_key_t*)pj_pool_zalloc(pool, sizeof(pj_ioqueue_key_t));
#endif

    rc = ioqueue_init_key(pool, ioqueue, key, sock, grp_lock, user_data, cb);
    if (rc != PJ_SUCCESS) {
	key = NULL;
	goto on_return;
    }

    /* 创建 key 的互斥量 */
 /*   rc = pj_mutex_create_recursive(pool, NULL, &key->mutex);
    if (rc != PJ_SUCCESS) {
	key = NULL;
	goto on_return;
    }
*/
    /* os_epoll_ctl. */
    ev.events = EPOLLIN | EPOLLERR;
    ev.epoll_data = (epoll_data_type)key;
    status = os_epoll_ctl(ioqueue->epfd, EPOLL_CTL_ADD, sock, &ev);
    if (status < 0) {
	rc = pj_get_os_error();
	pj_lock_destroy(key->lock);
	key = NULL;
	TRACE_((THIS_FILE,
                "pj_ioqueue_register_sock error: os_epoll_ctl rc=%d",
                status));
	goto on_return;
    }

    /* Register */
    pj_list_insert_before(&ioqueue->active_list, key);
    ++ioqueue->count;

    //TRACE_((THIS_FILE, "socket registered, count=%d", ioqueue->count));

on_return:
    if (rc != PJ_SUCCESS) {
	if (key && key->grp_lock)
	    pj_grp_lock_dec_ref_dbg(key->grp_lock, "ioqueue", 0);
    }
    *p_key = key;
    pj_lock_release(ioqueue->lock);

    return rc;
}

PJ_DEF(pj_status_t) pj_ioqueue_register_sock( pj_pool_t *pool,
					      pj_ioqueue_t *ioqueue,
					      pj_sock_t sock,
					      void *user_data,
					      const pj_ioqueue_callback *cb,
					      pj_ioqueue_key_t **p_key)
{
    return pj_ioqueue_register_sock2(pool, ioqueue, sock, NULL, user_data,
                                     cb, p_key);
}

#if PJ_IOQUEUE_HAS_SAFE_UNREG
/* 递增键的参考计数器 */
static void increment_counter(pj_ioqueue_key_t *key)
{
    pj_mutex_lock(key->ioqueue->ref_cnt_mutex);
    ++key->ref_count;
    pj_mutex_unlock(key->ioqueue->ref_cnt_mutex);
}

/*
 * 减少键的参考计数器，当计数器达到零时，销毁键
 *
 * 注意：持有ioqueue的锁时不能调用此函数
 */
static void decrement_counter(pj_ioqueue_key_t *key)
{
    pj_lock_acquire(key->ioqueue->lock);
    pj_mutex_lock(key->ioqueue->ref_cnt_mutex);
    --key->ref_count;
    if (key->ref_count == 0) {

	pj_assert(key->closing == 1);
	pj_gettickcount(&key->free_time);
	key->free_time.msec += PJ_IOQUEUE_KEY_FREE_DELAY;
	pj_time_val_normalize(&key->free_time);

	pj_list_erase(key);
	pj_list_push_back(&key->ioqueue->closing_list, key);

    }
    pj_mutex_unlock(key->ioqueue->ref_cnt_mutex);
    pj_lock_release(key->ioqueue->lock);
}
#endif

/*
 * pj_ioqueue_unregister()
 *
 * 从ioqueue注销句柄。
 */
PJ_DEF(pj_status_t) pj_ioqueue_unregister( pj_ioqueue_key_t *key)
{
    pj_ioqueue_t *ioqueue;
    struct epoll_event ev;
    int status;

    PJ_ASSERT_RETURN(key != NULL, PJ_EINVAL);

    ioqueue = key->ioqueue;

    /* 锁定密钥以确保没有回调同时修改密钥。我们需要在 ioqueue 之前锁定密钥以防止死锁
     */
    pj_ioqueue_lock_key(key);

    /* 尽最大努力避免双键注销 */
    if (IS_CLOSING(key)) {
	pj_ioqueue_unlock_key(key);
	return PJ_SUCCESS;
    }

    /* 同时锁定ioqueue */
    pj_lock_acquire(ioqueue->lock);

    /* 避免“负的”ioqueue计数 */
    if (ioqueue->count > 0) {
	--ioqueue->count;
    } else {
	/* 如果发生这种情况，很可能会出现密钥的双重注销
	 */
	pj_assert(!"Bad ioqueue count in key unregistration!");
	PJ_LOG(1,(THIS_FILE, "Bad ioqueue count in key unregistration!"));
    }

#if !PJ_IOQUEUE_HAS_SAFE_UNREG
    pj_list_erase(key);
#endif

    ev.events = 0;
    ev.epoll_data = (epoll_data_type)key;
    status = os_epoll_ctl( ioqueue->epfd, EPOLL_CTL_DEL, key->fd, &ev);
    if (status != 0) {
	pj_status_t rc = pj_get_os_error();
	pj_lock_release(ioqueue->lock);
	pj_ioqueue_unlock_key(key);
	return rc;
    }

    /* 销毁 key */
    pj_sock_close(key->fd);

    pj_lock_release(ioqueue->lock);


#if PJ_IOQUEUE_HAS_SAFE_UNREG
    /* 标记键正在关闭。 */
    key->closing = 1;

    /* 减量计数器。 */
    decrement_counter(key);

    /* 完成 */
    if (key->grp_lock) {
	/* 只需dec_ref 并解锁。我们将在别处将grp_lock设置为NULL */
	pj_grp_lock_t *grp_lock = key->grp_lock;
	//不要将 grp_lock 设置为NULL，否则另一个线程将崩溃。把它当作悬挂的指针，但这应该是安全的
	//key->grp_lock = NULL;
	pj_grp_lock_dec_ref_dbg(grp_lock, "ioqueue", 0);
	pj_grp_lock_release(grp_lock);
    } else {
	pj_ioqueue_unlock_key(key);
    }
#else
    if (key->grp_lock) {
	/* 将grp_lock设置为NULL并解锁*/
	pj_grp_lock_t *grp_lock = key->grp_lock;
	// 不要将 grp_lock 设置为NULL，否则另一个线程将崩溃。把它当作悬挂的指针，但这应该是安全的
	//key->grp_lock = NULL;
	pj_grp_lock_dec_ref_dbg(grp_lock, "ioqueue", 0);
	pj_grp_lock_release(grp_lock);
    } else {
	pj_ioqueue_unlock_key(key);
    }

    pj_lock_destroy(key->lock);
#endif

    return PJ_SUCCESS;
}

/* ioqueue_remove_from_set()
 * 从 ioqueue_dispatch_event() 调用此函数，以指示 ioqueue 从 ioqueue 为指定事件设置的描述符集中删除指定的描述符。
 */
static void ioqueue_remove_from_set( pj_ioqueue_t *ioqueue,
                                     pj_ioqueue_key_t *key,
                                     enum ioqueue_event_type event_type)
{
    if (event_type == WRITEABLE_EVENT) {
	struct epoll_event ev;

	ev.events = EPOLLIN | EPOLLERR;
	ev.epoll_data = (epoll_data_type)key;
	os_epoll_ctl( ioqueue->epfd, EPOLL_CTL_MOD, key->fd, &ev);
    }
}

/*
 * ioqueue_add_to_set()
 * 从pj_ioqueue_recv()、pj_ioqueue_send()等调用此函数，以指示 ioqueue 将指定的句柄添加到 ioqueue 为指定事件设置的描述符中。
 */
static void ioqueue_add_to_set( pj_ioqueue_t *ioqueue,
                                pj_ioqueue_key_t *key,
                                enum ioqueue_event_type event_type )
{
    if (event_type == WRITEABLE_EVENT) {
	struct epoll_event ev;

	ev.events = EPOLLIN | EPOLLOUT | EPOLLERR;
	ev.epoll_data = (epoll_data_type)key;
	os_epoll_ctl( ioqueue->epfd, EPOLL_CTL_MOD, key->fd, &ev);
    }
}

#if PJ_IOQUEUE_HAS_SAFE_UNREG
/* Scan closing keys to be put to free list again */
static void scan_closing_keys(pj_ioqueue_t *ioqueue)
{
    pj_time_val now;
    pj_ioqueue_key_t *h;

    pj_gettickcount(&now);
    h = ioqueue->closing_list.next;
    while (h != &ioqueue->closing_list) {
	pj_ioqueue_key_t *next = h->next;

	pj_assert(h->closing != 0);

	if (PJ_TIME_VAL_GTE(now, h->free_time)) {
	    pj_list_erase(h);
	    // Don't set grp_lock to NULL otherwise the other thread
	    // will crash. Just leave it as dangling pointer, but this
	    // should be safe
	    //h->grp_lock = NULL;
	    pj_list_push_back(&ioqueue->free_list, h);
	}
	h = next;
    }
}
#endif

/*
 * pj_ioqueue_poll()
 *
 */
PJ_DEF(int) pj_ioqueue_poll( pj_ioqueue_t *ioqueue, const pj_time_val *timeout)
{
    int i, count, event_cnt, processed_cnt;
    int msec;
    //struct epoll_event *events = ioqueue->events;
    //struct queue *queue = ioqueue->queue;
    enum { MAX_EVENTS = PJ_IOQUEUE_MAX_CAND_EVENTS };
    struct epoll_event events[MAX_EVENTS];
    struct queue queue[MAX_EVENTS];
    pj_timestamp t1, t2;

    PJ_CHECK_STACK();

    msec = timeout ? PJ_TIME_VAL_MSEC(*timeout) : 9000;

    TRACE_((THIS_FILE, "start os_epoll_wait, msec=%d", msec));
    pj_get_timestamp(&t1);

    //count = os_epoll_wait( ioqueue->epfd, events, ioqueue->max, msec);
    count = os_epoll_wait( ioqueue->epfd, events, MAX_EVENTS, msec);
    if (count == 0) {
#if PJ_IOQUEUE_HAS_SAFE_UNREG
    /* Check the closing keys only when there's no activity and when there are
     * pending closing keys.
     */
    if (count == 0 && !pj_list_empty(&ioqueue->closing_list)) {
	pj_lock_acquire(ioqueue->lock);
	scan_closing_keys(ioqueue);
	pj_lock_release(ioqueue->lock);
    }
#endif
	TRACE_((THIS_FILE, "os_epoll_wait timed out"));
	return count;
    }
    else if (count < 0) {
	TRACE_((THIS_FILE, "os_epoll_wait error"));
	return -pj_get_netos_error();
    }

    pj_get_timestamp(&t2);
    TRACE_((THIS_FILE, "os_epoll_wait returns %d, time=%d usec",
		       count, pj_elapsed_usec(&t1, &t2)));

    /* 锁 ioqueue. */
    pj_lock_acquire(ioqueue->lock);

    for (event_cnt=0, i=0; i<count; ++i) {
	pj_ioqueue_key_t *h = (pj_ioqueue_key_t*)(epoll_data_type)
				events[i].epoll_data;

	TRACE_((THIS_FILE, "event %d: events=%d", i, events[i].events));

	/*
	 * 检查可读性
	 */
	if ((events[i].events & EPOLLIN) &&
	    (key_has_pending_read(h) || key_has_pending_accept(h)) && !IS_CLOSING(h) ) {

#if PJ_IOQUEUE_HAS_SAFE_UNREG
	    increment_counter(h);
#endif
	    queue[event_cnt].key = h;
	    queue[event_cnt].event_type = READABLE_EVENT;
	    ++event_cnt;
	    continue;
	}

	/*
	 * 检查可写性
	 */
	if ((events[i].events & EPOLLOUT) && key_has_pending_write(h) && !IS_CLOSING(h)) {

#if PJ_IOQUEUE_HAS_SAFE_UNREG
	    increment_counter(h);
#endif
	    queue[event_cnt].key = h;
	    queue[event_cnt].event_type = WRITEABLE_EVENT;
	    ++event_cnt;
	    continue;
	}

#if PJ_HAS_TCP
	/*
	 * 检查 connect() 操作是否完成
	 */
	if ((events[i].events & EPOLLOUT) && (h->connecting) && !IS_CLOSING(h)) {

#if PJ_IOQUEUE_HAS_SAFE_UNREG
	    increment_counter(h);
#endif
	    queue[event_cnt].key = h;
	    queue[event_cnt].event_type = WRITEABLE_EVENT;
	    ++event_cnt;
	    continue;
	}
#endif /* PJ_HAS_TCP */

	/*
	 * 检查错误情况
	 */
	if ((events[i].events & EPOLLERR) && !IS_CLOSING(h)) {
	    /*
	     * 我们需要处理这个异常事件。如果与我们的连接有关，请如实报告。如果不是，只需将其报告为读取事件，更高的层将处理它。
	     */
	    if (h->connecting) {
#if PJ_IOQUEUE_HAS_SAFE_UNREG
		increment_counter(h);
#endif
		queue[event_cnt].key = h;
		queue[event_cnt].event_type = EXCEPTION_EVENT;
		++event_cnt;
	    } else if (key_has_pending_read(h) || key_has_pending_accept(h)) {
#if PJ_IOQUEUE_HAS_SAFE_UNREG
		increment_counter(h);
#endif
		queue[event_cnt].key = h;
		queue[event_cnt].event_type = READABLE_EVENT;
		++event_cnt;
	    }
	    continue;
	}
    }
    for (i=0; i<event_cnt; ++i) {
	if (queue[i].key->grp_lock)
	    pj_grp_lock_add_ref_dbg(queue[i].key->grp_lock, "ioqueue", 0);
    }

    PJ_RACE_ME(5);

    pj_lock_release(ioqueue->lock);

    PJ_RACE_ME(5);

    processed_cnt = 0;

    /* 现在处理事件 */
    for (i=0; i<event_cnt; ++i) {

	/* 只需在单个轮询中不超过PJ_IOQUEUE_MAX_EVENTS_IN_SINGLE_POLL即可 */
	if (processed_cnt < PJ_IOQUEUE_MAX_EVENTS_IN_SINGLE_POLL) {
	    switch (queue[i].event_type) {
	    case READABLE_EVENT:
		if (ioqueue_dispatch_read_event(ioqueue, queue[i].key))
		    ++processed_cnt;
		break;
	    case WRITEABLE_EVENT:
		if (ioqueue_dispatch_write_event(ioqueue, queue[i].key))
		    ++processed_cnt;
		break;
	    case EXCEPTION_EVENT:
		if (ioqueue_dispatch_exception_event(ioqueue, queue[i].key))
		    ++processed_cnt;
		break;
	    case NO_EVENT:
		pj_assert(!"Invalid event!");
		break;
	    }
	}

#if PJ_IOQUEUE_HAS_SAFE_UNREG
	decrement_counter(queue[i].key);
#endif

	if (queue[i].key->grp_lock)
	    pj_grp_lock_dec_ref_dbg(queue[i].key->grp_lock,
	                            "ioqueue", 0);
    }

    /*
     * 特殊情况：
     *      当epoll返回 >0 但没有实际设置描述符时
     */
    if (count > 0 && !event_cnt && msec > 0) {
	pj_thread_sleep(msec);
    }

    TRACE_((THIS_FILE, "     poll: count=%d events=%d processed=%d",
		       count, event_cnt, processed_cnt));

    pj_get_timestamp(&t1);
    TRACE_((THIS_FILE, "ioqueue_poll() returns %d, time=%d usec",
		       processed, pj_elapsed_usec(&t2, &t1)));

    return processed_cnt;
}

