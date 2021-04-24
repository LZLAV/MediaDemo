/**
 * 已完成:
 *      1. 创建 ioqueue   / 销毁 ioqueue
 *      2. 注册 socket 句柄到 ioqueue    / socket 句柄从 ioqueue 中注销
 *      3. 从队列中移除指定key,type 的事件
 *      4. 添加指定 key,type 的事件到 ioqueue
 *      5. ioqueue_poll() 轮询事件  （pj_sock_select()）
 *
 */

/*
 * sock_select.c
 *
 * 这是使用 pj_sock_select() 实现的IOQueue。它运行在 pj_sock_select() 可用的任何地方（当前为Win32、Linux、Linux内核等）。
 */

#include <pj/ioqueue.h>
#include <pj/os.h>
#include <pj/lock.h>
#include <pj/log.h>
#include <pj/list.h>
#include <pj/pool.h>
#include <pj/string.h>
#include <pj/assert.h>
#include <pj/sock.h>
#include <pj/compat/socket.h>
#include <pj/sock_select.h>
#include <pj/sock_qos.h>
#include <pj/errno.h>
#include <pj/rand.h>

/*
 * 现在我们可以访问OS'es<sys/select>，让我们再次检查PJ_IOQUEUE_MAX_HANDLES是否大于FD_SETSIZE
 */
#if PJ_IOQUEUE_MAX_HANDLES > FD_SETSIZE
#   error "PJ_IOQUEUE_MAX_HANDLES cannot be greater than FD_SETSIZE"
#endif


/*
 * 包含来自公共抽象的声明
 */
#include "ioqueue_common_abs.h"

/*
 * 与 ioqueue_select() 有关的问题
 *
 * recv()中的 EAGAIN/EWOULDBLOCK错误：
 *  -当多个线程使用 ioqueue 时，应用程序可能会在接收回调中接收EAGAIN或EWOULDBLOCK
 *  发生此错误的原因是有多个线程正在监视同一描述符集，因此当所有线程同时调用 recv()或recvfrom()时，只有一个线程成功，其余线程将获得错误
 *
 */
#define THIS_FILE   "ioq_select"

/*
 * select ioqueue 依赖socket函数（pj_sock_xxx()）返回正确的错误代码。
 */
#if PJ_RETURN_OS_ERROR(100) != PJ_STATUS_FROM_OS(100)
#   error "Error reporting must be enabled for this function to work!"
#endif

/*
 * 在调试生成期间,VALIDATE_FD_SET是否已设置。这将检查 fd_sets 的有效性。
 */
/*
#if defined(PJ_DEBUG) && PJ_DEBUG != 0
#  define VALIDATE_FD_SET		1
#else
#  define VALIDATE_FD_SET		0
#endif
*/
#define VALIDATE_FD_SET     0

#if 0
#  define TRACE__(args)	PJ_LOG(3,args)
#else
#  define TRACE__(args)
#endif

/*
 * 键的描述
 */
struct pj_ioqueue_key_t {
    DECLARE_COMMON_KEY
};

/*
 * I/O队列的描述
 */
struct pj_ioqueue_t {
    DECLARE_COMMON_IOQUEUE

    unsigned max, count;    /* 最大和当前密钥计数	    */
    int nfds;        /* 最大fd值（对于select） */
    pj_ioqueue_key_t active_list;    /* 活动键列表		    */
    pj_fd_set_t rfdset;
    pj_fd_set_t wfdset;
#if PJ_HAS_TCP
    pj_fd_set_t xfdset;
#endif

#if PJ_IOQUEUE_HAS_SAFE_UNREG
    pj_mutex_t *ref_cnt_mutex;
    pj_ioqueue_key_t closing_list;
    pj_ioqueue_key_t free_list;
#endif
};

/* Proto */
#if defined(PJ_IPHONE_OS_HAS_MULTITASKING_SUPPORT) && \
        PJ_IPHONE_OS_HAS_MULTITASKING_SUPPORT != 0
static pj_status_t replace_udp_sock(pj_ioqueue_key_t *h);
#endif

/*
 * 在声明pj_ioqueue_key_t和pj_ioqueue_t之后，包含公共抽象的实现
 */
#include "ioqueue_common_abs.c"

#if PJ_IOQUEUE_HAS_SAFE_UNREG

/* 再次扫描要放入空闲列表的关闭键 */
static void scan_closing_keys(pj_ioqueue_t *ioqueue);

#endif

/*
 * pj_ioqueue_name()
 */
PJ_DEF(const char*)pj_ioqueue_name(void) {
    return "select";
}

/* 
 * 扫描 socket 描述符集以查找最大的 descriptor.This select() 需要值
 */
#if defined(PJ_SELECT_NEEDS_NFDS) && PJ_SELECT_NEEDS_NFDS != 0
static void rescan_fdset(pj_ioqueue_t *ioqueue)
{
    pj_ioqueue_key_t *key = ioqueue->active_list.next;
    int max = 0;

    while (key != &ioqueue->active_list) {
    if (key->fd > max)
        max = key->fd;
    key = key->next;
    }

    ioqueue->nfds = max;
}
#else

static void rescan_fdset(pj_ioqueue_t *ioqueue) {
    ioqueue->nfds = FD_SETSIZE - 1;
}

#endif


/*
 * pj_ioqueue_create()
 *
 * 创建 select 队列
 */
PJ_DEF(pj_status_t) pj_ioqueue_create(pj_pool_t *pool,
                                      pj_size_t max_fd,
                                      pj_ioqueue_t **p_ioqueue) {
    pj_ioqueue_t *ioqueue;
    pj_lock_t *lock;
    unsigned i;
    pj_status_t rc;

    /* 检查参数是否有效。 */
    PJ_ASSERT_RETURN(pool != NULL && p_ioqueue != NULL &&
                     max_fd > 0 && max_fd <= PJ_IOQUEUE_MAX_HANDLES,
                     PJ_EINVAL);

    /* 检查 pj_ioqueue_op_key_t 的大小是否足够 */
    PJ_ASSERT_RETURN(sizeof(pj_ioqueue_op_key_t) - sizeof(void *) >=
                     sizeof(union operation_key), PJ_EBUG);

    /* 创建和初始化公共ioqueue  */
    ioqueue = PJ_POOL_ALLOC_T(pool, pj_ioqueue_t);
    ioqueue_init(ioqueue);

    ioqueue->max = (unsigned) max_fd;
    ioqueue->count = 0;
    PJ_FD_ZERO(&ioqueue->rfdset);
    PJ_FD_ZERO(&ioqueue->wfdset);
#if PJ_HAS_TCP
    PJ_FD_ZERO(&ioqueue->xfdset);
#endif
    pj_list_init(&ioqueue->active_list);

    rescan_fdset(ioqueue);

#if PJ_IOQUEUE_HAS_SAFE_UNREG
    /* When safe unregistration is used (the default), we pre-create
     * all keys and put them in the free list.
     */

    /* Mutex to protect key's reference counter 
     * We don't want to use key's mutex or ioqueue's mutex because
     * that would create deadlock situation in some cases.
     */
    rc = pj_mutex_create_simple(pool, NULL, &ioqueue->ref_cnt_mutex);
    if (rc != PJ_SUCCESS)
        return rc;


    /* Init key list */
    pj_list_init(&ioqueue->free_list);
    pj_list_init(&ioqueue->closing_list);


    /* Pre-create all keys according to max_fd */
    for (i = 0; i < max_fd; ++i) {
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

    /* 创建和初始化队列 mutex */
    rc = pj_lock_create_simple_mutex(pool, "ioq%p", &lock);
    if (rc != PJ_SUCCESS)
        return rc;

    rc = pj_ioqueue_set_lock(ioqueue, lock, PJ_TRUE);
    if (rc != PJ_SUCCESS)
        return rc;

    PJ_LOG(4, ("pjlib", "select() I/O Queue created (%p)", ioqueue));

    *p_ioqueue = ioqueue;
    return PJ_SUCCESS;
}

/*
 * pj_ioqueue_destroy()
 *
 * 销毁 ioqueue.
 */
PJ_DEF(pj_status_t) pj_ioqueue_destroy(pj_ioqueue_t *ioqueue) {
    pj_ioqueue_key_t *key;

    PJ_ASSERT_RETURN(ioqueue, PJ_EINVAL);

    pj_lock_acquire(ioqueue->lock);

#if PJ_IOQUEUE_HAS_SAFE_UNREG
    /* 销毁引用计数器 */
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
 * 注册 socket 句柄到队列
 */
PJ_DEF(pj_status_t) pj_ioqueue_register_sock2(pj_pool_t *pool,
                                              pj_ioqueue_t *ioqueue,
                                              pj_sock_t sock,
                                              pj_grp_lock_t *grp_lock,
                                              void *user_data,
                                              const pj_ioqueue_callback *cb,
                                              pj_ioqueue_key_t **p_key) {
    pj_ioqueue_key_t *key = NULL;
#if defined(PJ_WIN32) && PJ_WIN32 != 0 || \
    defined(PJ_WIN64) && PJ_WIN64 != 0 || \
    defined(PJ_WIN32_WINCE) && PJ_WIN32_WINCE != 0
    u_long value;
#else
    pj_uint32_t value;
#endif
    pj_status_t rc = PJ_SUCCESS;

    PJ_ASSERT_RETURN(pool && ioqueue && sock != PJ_INVALID_SOCKET &&
                     cb && p_key, PJ_EINVAL);

    /* 在具有包含fd bitmap的fd_set的平台上，如*nix family，当给定的fd大于FD_SETSIZE时，请避免select()导致的潜在内存损坏。
     */
    if (sizeof(fd_set) < FD_SETSIZE && sock >= FD_SETSIZE) {
        PJ_LOG(4, ("pjlib", "Failed to register socket to ioqueue because "
                            "socket fd is too big (fd=%d/FD_SETSIZE=%d)",
                sock, FD_SETSIZE));
        return PJ_ETOOBIG;
    }

    pj_lock_acquire(ioqueue->lock);

    if (ioqueue->count >= ioqueue->max) {
        rc = PJ_ETOOMANY;
        goto on_return;
    }

    /* 如果使用了安全注销（PJ_IOQUEUE_HAS_SAFE_UNREG），则从空闲列表中获取密钥。否则，分配一个新的
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
    key = (pj_ioqueue_key_t*)pj_pool_zalloc(pool, sizeof(pj_ioqueue_key_t));
#endif

    rc = ioqueue_init_key(pool, ioqueue, key, sock, grp_lock, user_data, cb);
    if (rc != PJ_SUCCESS) {
        key = NULL;
        goto on_return;
    }

    /* 将套接字设置为非阻塞。 */
    value = 1;
#if defined(PJ_WIN32) && PJ_WIN32 != 0 || \
    defined(PJ_WIN64) && PJ_WIN64 != 0 || \
    defined(PJ_WIN32_WINCE) && PJ_WIN32_WINCE != 0
    if (ioctlsocket(sock, FIONBIO, &value)) {
#else
    if (ioctl(sock, FIONBIO, &value)) {
#endif
        rc = pj_get_netos_error();
        goto on_return;
    }


    /* 放入活动列表 */
    pj_list_insert_before(&ioqueue->active_list, key);
    ++ioqueue->count;

    /* 重新扫描fdset以获取最大描述符 */
    rescan_fdset(ioqueue);

    on_return:
    /* 出错时，套接字可能处于非阻塞模式 */
    if (rc != PJ_SUCCESS) {
        if (key && key->grp_lock)
            pj_grp_lock_dec_ref_dbg(key->grp_lock, "ioqueue", 0);
    }
    *p_key = key;
    pj_lock_release(ioqueue->lock);

    return rc;
}

PJ_DEF(pj_status_t) pj_ioqueue_register_sock(pj_pool_t *pool,
                                             pj_ioqueue_t *ioqueue,
                                             pj_sock_t sock,
                                             void *user_data,
                                             const pj_ioqueue_callback *cb,
                                             pj_ioqueue_key_t **p_key) {
    return pj_ioqueue_register_sock2(pool, ioqueue, sock, NULL, user_data,
                                     cb, p_key);
}

#if PJ_IOQUEUE_HAS_SAFE_UNREG

/* 递增key 的参考计数器 */
static void increment_counter(pj_ioqueue_key_t *key) {
    pj_mutex_lock(key->ioqueue->ref_cnt_mutex);
    ++key->ref_count;
    pj_mutex_unlock(key->ioqueue->ref_cnt_mutex);
}

/*
 * 减少 key 的参考计数器，当计数器达到零时，销毁key
 * 注意：持有 ioqueue 的锁时不能调用此函数
 */
static void decrement_counter(pj_ioqueue_key_t *key) {
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
        /* Rescan fdset to get max descriptor */
        rescan_fdset(key->ioqueue);
    }
    pj_mutex_unlock(key->ioqueue->ref_cnt_mutex);
    pj_lock_release(key->ioqueue->lock);
}

#endif


/*
 * pj_ioqueue_unregister()
 *
 * 从 ioqueue 注销句柄。
 */
PJ_DEF(pj_status_t) pj_ioqueue_unregister(pj_ioqueue_key_t *key) {
    pj_ioqueue_t *ioqueue;

    PJ_ASSERT_RETURN(key, PJ_EINVAL);

    ioqueue = key->ioqueue;

    /* 锁定密钥以确保没有回调同时修改密钥。我们需要在 ioqueue 之前锁定密钥以防止死锁。
     */
    pj_ioqueue_lock_key(key);

    /* 尽最大努力避免多次键注销 */
    if (IS_CLOSING(key)) {
        pj_ioqueue_unlock_key(key);
        return PJ_SUCCESS;
    }

    /* 同时锁定ioqueue */
    pj_lock_acquire(ioqueue->lock);

    /* 避免负数 ioqueue 数量 */
    if (ioqueue->count > 0) {
        --ioqueue->count;
    } else {
        /*
         * 如果发生这种情况，很可能会出现密钥的多次注销
         */
        pj_assert(!"Bad ioqueue count in key unregistration!");
        PJ_LOG(1, (THIS_FILE, "Bad ioqueue count in key unregistration!"));
    }

#if !PJ_IOQUEUE_HAS_SAFE_UNREG
    /* #520, key 多次删除 */
    pj_list_erase(key);
#endif
    PJ_FD_CLR(key->fd, &ioqueue->rfdset);
    PJ_FD_CLR(key->fd, &ioqueue->wfdset);
#if PJ_HAS_TCP
    PJ_FD_CLR(key->fd, &ioqueue->xfdset);
#endif

    /* 关闭 socket. */
    if (key->fd != PJ_INVALID_SOCKET) {
        pj_sock_close(key->fd);
        key->fd = PJ_INVALID_SOCKET;
    }

    /* 清除回调 */
    key->cb.on_accept_complete = NULL;
    key->cb.on_connect_complete = NULL;
    key->cb.on_read_complete = NULL;
    key->cb.on_write_complete = NULL;

    /* 必须先释放ioqueue锁，然后再递减计数器，以防止死锁。
     */
    pj_lock_release(ioqueue->lock);

#if PJ_IOQUEUE_HAS_SAFE_UNREG
    /* 标记键正在关闭 */
    key->closing = 1;

    /* 减量计数器 */
    decrement_counter(key);

    /* 完成 */
    if (key->grp_lock) {
        /* 只需松开并解锁。我们将在别处将grp_lock设置为NULL*/
        pj_grp_lock_t *grp_lock = key->grp_lock;
        //不要将grp_lock设置为NULL，否则另一个线程将崩溃。把它当作悬挂的指针，但这应该是安全的
        //key->grp_lock = NULL;
        pj_grp_lock_dec_ref_dbg(grp_lock, "ioqueue", 0);
        pj_grp_lock_release(grp_lock);
    } else {
        pj_ioqueue_unlock_key(key);
    }
#else
    if (key->grp_lock) {
    /* 设置 grp_lock为 NULL 并解锁 */
    pj_grp_lock_t *grp_lock = key->grp_lock;
    //不要将 grp_lock 设置为NULL，否则另一个线程将崩溃。把它当作悬挂的指针，但这应该是安全的
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


/*
 * 这是为了检查fd_set值是否与每个键中当前设置的操作一致。
 */
#if VALIDATE_FD_SET
static void validate_sets(const pj_ioqueue_t *ioqueue,
              const pj_fd_set_t *rfdset,
              const pj_fd_set_t *wfdset,
              const pj_fd_set_t *xfdset)
{
    pj_ioqueue_key_t *key;

    /*
     * 这基本上不起作用了。
     * 我们需要在执行检查之前锁定密钥，但不能这样做，因为我们持有ioqueue mutex。如果我们现在获取密钥的互斥，将导致死锁。
     */
    pj_assert(0);

    key = ioqueue->active_list.next;
    while (key != &ioqueue->active_list) {
    if (!pj_list_empty(&key->read_list)
#if defined(PJ_HAS_TCP) && PJ_HAS_TCP != 0
        || !pj_list_empty(&key->accept_list)
#endif
        )
    {
        pj_assert(PJ_FD_ISSET(key->fd, rfdset));
    }
    else {
        pj_assert(PJ_FD_ISSET(key->fd, rfdset) == 0);
    }
    if (!pj_list_empty(&key->write_list)
#if defined(PJ_HAS_TCP) && PJ_HAS_TCP != 0
        || key->connecting
#endif
       )
    {
        pj_assert(PJ_FD_ISSET(key->fd, wfdset));
    }
    else {
        pj_assert(PJ_FD_ISSET(key->fd, wfdset) == 0);
    }
#if defined(PJ_HAS_TCP) && PJ_HAS_TCP != 0
    if (key->connecting)
    {
        pj_assert(PJ_FD_ISSET(key->fd, xfdset));
    }
    else {
        pj_assert(PJ_FD_ISSET(key->fd, xfdset) == 0);
    }
#endif /* PJ_HAS_TCP */

    key = key->next;
    }
}
#endif    /* VALIDATE_FD_SET */


/* ioqueue_remove_from_set()
 * 从 ioqueue_dispatch_event() 调用此函数，以指示ioqueue从ioqueue为指定事件设置的描述符集中删除指定的描述符
 */
static void ioqueue_remove_from_set(pj_ioqueue_t *ioqueue,
                                    pj_ioqueue_key_t *key,
                                    enum ioqueue_event_type event_type) {
    pj_lock_acquire(ioqueue->lock);

    if (event_type == READABLE_EVENT)
        PJ_FD_CLR((pj_sock_t) key->fd, &ioqueue->rfdset);
    else if (event_type == WRITEABLE_EVENT)
        PJ_FD_CLR((pj_sock_t) key->fd, &ioqueue->wfdset);
#if defined(PJ_HAS_TCP) && PJ_HAS_TCP != 0
    else if (event_type == EXCEPTION_EVENT)
        PJ_FD_CLR((pj_sock_t) key->fd, &ioqueue->xfdset);
#endif
    else
        pj_assert(0);

    pj_lock_release(ioqueue->lock);
}

/*
 * ioqueue_add_to_set()
 * 从pj_ioqueue_recv()、pj_ioqueue_send()等调用此函数，以指示ioqueue将指定的句柄添加到ioqueue为指定事件设置的描述符中
 */
static void ioqueue_add_to_set(pj_ioqueue_t *ioqueue,
                               pj_ioqueue_key_t *key,
                               enum ioqueue_event_type event_type) {
    pj_lock_acquire(ioqueue->lock);

    if (event_type == READABLE_EVENT)
        PJ_FD_SET((pj_sock_t) key->fd, &ioqueue->rfdset);
    else if (event_type == WRITEABLE_EVENT)
        PJ_FD_SET((pj_sock_t) key->fd, &ioqueue->wfdset);
#if defined(PJ_HAS_TCP) && PJ_HAS_TCP != 0
    else if (event_type == EXCEPTION_EVENT)
        PJ_FD_SET((pj_sock_t) key->fd, &ioqueue->xfdset);
#endif
    else
        pj_assert(0);

    pj_lock_release(ioqueue->lock);
}

#if PJ_IOQUEUE_HAS_SAFE_UNREG

/* 再次扫描要放入空闲列表的关闭键 */
static void scan_closing_keys(pj_ioqueue_t *ioqueue) {
    pj_time_val now;
    pj_ioqueue_key_t *h;

    pj_gettickcount(&now);
    h = ioqueue->closing_list.next;
    while (h != &ioqueue->closing_list) {
        pj_ioqueue_key_t *next = h->next;

        pj_assert(h->closing != 0);

        if (PJ_TIME_VAL_GTE(now, h->free_time)) {
            pj_list_erase(h);
            //不要将 grp_lock 设置为NULL，否则另一个线程将崩溃。把它当作悬挂的指针，但这应该是安全的
            //h->grp_lock = NULL;
            pj_list_push_back(&ioqueue->free_list, h);
        }
        h = next;
    }
}

#endif

#if defined(PJ_IPHONE_OS_HAS_MULTITASKING_SUPPORT) && \
    PJ_IPHONE_OS_HAS_MULTITASKING_SUPPORT != 0
static pj_status_t replace_udp_sock(pj_ioqueue_key_t *h)
{
    enum flags {
    HAS_PEER_ADDR = 1,
    HAS_QOS = 2
    };
    pj_sock_t old_sock, new_sock = PJ_INVALID_SOCKET;
    pj_sockaddr local_addr, rem_addr;
    int val, addr_len;
    pj_fd_set_t *fds[3];
    unsigned i, fds_cnt, flags=0;
    pj_qos_params qos_params;
    unsigned msec;
    pj_status_t status;

    pj_lock_acquire(h->ioqueue->lock);

    old_sock = h->fd;

    fds_cnt = 0;
    fds[fds_cnt++] = &h->ioqueue->rfdset;
    fds[fds_cnt++] = &h->ioqueue->wfdset;
#if PJ_HAS_TCP
    fds[fds_cnt++] = &h->ioqueue->xfdset;
#endif

    /* 只能替换UDP套接字 */
    pj_assert(h->fd_type == pj_SOCK_DGRAM());

    PJ_LOG(4,(THIS_FILE, "Attempting to replace UDP socket %d", old_sock));

    for (msec=20; (msec<1000 && status != PJ_SUCCESS) ;
         msec<1000? msec=msec*2 : 1000)
    {
        if (msec > 20) {
            PJ_LOG(4,(THIS_FILE, "Retry to replace UDP socket %d", old_sock));
            pj_thread_sleep(msec);
        }

        if (old_sock != PJ_INVALID_SOCKET) {
            /* 检查老 socket */
            addr_len = sizeof(local_addr);
            status = pj_sock_getsockname(old_sock, &local_addr, &addr_len);
            if (status != PJ_SUCCESS) {
                PJ_LOG(5,(THIS_FILE, "Error get socket name %d", status));
                continue;
            }

            addr_len = sizeof(rem_addr);
            status = pj_sock_getpeername(old_sock, &rem_addr, &addr_len);
            if (status != PJ_SUCCESS) {
                PJ_LOG(5,(THIS_FILE, "Error get peer name %d", status));
            } else {
                flags |= HAS_PEER_ADDR;
            }

            status = pj_sock_get_qos_params(old_sock, &qos_params);
            if (status == PJ_STATUS_FROM_OS(EBADF) ||
                status == PJ_STATUS_FROM_OS(EINVAL))
            {
                PJ_LOG(5,(THIS_FILE, "Error get qos param %d", status));
                continue;
            }

            if (status != PJ_SUCCESS) {
                PJ_LOG(5,(THIS_FILE, "Error get qos param %d", status));
            } else {
                flags |= HAS_QOS;
            }

            /* 我们已经处理完旧的套接字，请关闭它，否则将在bind()中出错
             */
            status = pj_sock_close(old_sock);
               if (status != PJ_SUCCESS) {
                PJ_LOG(5,(THIS_FILE, "Error closing socket %d", status));
            }

            old_sock = PJ_INVALID_SOCKET;
        }

        /* 准备新 socket */
        status = pj_sock_socket(local_addr.addr.sa_family, PJ_SOCK_DGRAM, 0,
                                &new_sock);
        if (status != PJ_SUCCESS) {
            PJ_LOG(5,(THIS_FILE, "Error create socket %d", status));
            continue;
        }

        /*
         * 即使在套接字关闭之后，我们仍然会得到“Address in use”错误，因此请使用SO_REUSEADDR强制它
         */
        val = 1;
        status = pj_sock_setsockopt(new_sock, SOL_SOCKET, SO_REUSEADDR,
                                    &val, sizeof(val));
        if (status == PJ_STATUS_FROM_OS(EBADF) ||
            status == PJ_STATUS_FROM_OS(EINVAL))
        {
            PJ_LOG(5,(THIS_FILE, "Error set socket option %d",
                      status));
            continue;
        }

        /* 循环是愚蠢的，但我们还能做什么呢？ */
        addr_len = pj_sockaddr_get_len(&local_addr);
        for (msec=20; msec<1000 ; msec<1000? msec=msec*2 : 1000) {
            status = pj_sock_bind(new_sock, &local_addr, addr_len);
            if (status != PJ_STATUS_FROM_OS(EADDRINUSE))
                break;
            PJ_LOG(4,(THIS_FILE, "Address is still in use, retrying.."));
            pj_thread_sleep(msec);
        }

        if (status != PJ_SUCCESS)
            continue;

        if (flags & HAS_QOS) {
            status = pj_sock_set_qos_params(new_sock, &qos_params);
            if (status == PJ_STATUS_FROM_OS(EINVAL)) {
                PJ_LOG(5,(THIS_FILE, "Error set qos param %d", status));
                continue;
            }
        }

        if (flags & HAS_PEER_ADDR) {
            status = pj_sock_connect(new_sock, &rem_addr, addr_len);
            if (status != PJ_SUCCESS) {
                PJ_LOG(5,(THIS_FILE, "Error connect socket %d", status));
                continue;
            }
        }
    }

    if (status != PJ_SUCCESS)
        goto on_error;

    /* 将套接字设置为非阻塞 */
    val = 1;
#if defined(PJ_WIN32) && PJ_WIN32!=0 || \
    defined(PJ_WIN64) && PJ_WIN64 != 0 || \
    defined(PJ_WIN32_WINCE) && PJ_WIN32_WINCE!=0
    if (ioctlsocket(new_sock, FIONBIO, &val)) {
#else
    if (ioctl(new_sock, FIONBIO, &val)) {
#endif
        status = pj_get_netos_error();
    goto on_error;
    }

    /* 将fd集合中出现的旧套接字替换为新套接字
     */
    for (i=0; i<fds_cnt; ++i) {
    if (PJ_FD_ISSET(h->fd, fds[i])) {
        PJ_FD_CLR(h->fd, fds[i]);
        PJ_FD_SET(new_sock, fds[i]);
    }
    }

    /* 最后替换键中的fd */
    h->fd = new_sock;

    PJ_LOG(4,(THIS_FILE, "UDP has been replaced successfully!"));

    pj_lock_release(h->ioqueue->lock);

    return PJ_SUCCESS;

on_error:
    if (new_sock != PJ_INVALID_SOCKET)
    pj_sock_close(new_sock);
    if (old_sock != PJ_INVALID_SOCKET)
        pj_sock_close(old_sock);

    /* 清除fd集合中出现的旧套接字 */
    for (i=0; i<fds_cnt; ++i) {
    if (PJ_FD_ISSET(h->fd, fds[i])) {
        PJ_FD_CLR(h->fd, fds[i]);
    }
    }

    h->fd = PJ_INVALID_SOCKET;
    PJ_PERROR(1,(THIS_FILE, status, "Error replacing socket [%d]", status));
    pj_lock_release(h->ioqueue->lock);
    return PJ_ESOCKETSTOP;
}
#endif


/*
 * pj_ioqueue_poll()
 *
 * 有几件事值得写：
 *      -我们过去每次轮询只调用一次回调，但效果不太好。原因是在某些情况下，写回调总是被调用，因此不给读回调以被调用。
 *      例如，当用户在write回调中提交write操作时，就会发生这种情况。
 *
 *      结果，我们改变了行为，使得现在在一次轮询中调用多个回调。它应该也很快，只是我们需要小心ioqueue数据结构。
 *
 *      -为了保证抢占性等，poll函数必须严格作用于ioqueue的fd_set副本（而不是原始副本）。
 */
PJ_DEF(int) pj_ioqueue_poll(pj_ioqueue_t *ioqueue, const pj_time_val *timeout) {
    pj_fd_set_t rfdset, wfdset, xfdset;
    int nfds;
    int i, count, event_cnt, processed_cnt;
    pj_ioqueue_key_t *h;
    enum {
        MAX_EVENTS = PJ_IOQUEUE_MAX_CAND_EVENTS
    };
    struct event {
        pj_ioqueue_key_t *key;
        enum ioqueue_event_type event_type;
    } event[MAX_EVENTS];

    PJ_ASSERT_RETURN(ioqueue, -PJ_EINVAL);

    /* 在拷贝fd_set之前锁定ioqueue */
    pj_lock_acquire(ioqueue->lock);

    /* 我们将只在有要轮询的套接字时执行select()
     * 否则select()将返回错误
     */
    if (PJ_FD_COUNT(&ioqueue->rfdset) == 0 &&
        PJ_FD_COUNT(&ioqueue->wfdset) == 0
        #if defined(PJ_HAS_TCP) && PJ_HAS_TCP != 0
        && PJ_FD_COUNT(&ioqueue->xfdset) == 0
#endif
            ) {
#if PJ_IOQUEUE_HAS_SAFE_UNREG
        scan_closing_keys(ioqueue);
#endif
        pj_lock_release(ioqueue->lock);
        TRACE__((THIS_FILE, "     poll: no fd is set"));
        if (timeout)
            pj_thread_sleep(PJ_TIME_VAL_MSEC(*timeout));
        return 0;
    }

    /* 将ioqueue的pj_fd_set_t复制到局部变量 */
    pj_memcpy(&rfdset, &ioqueue->rfdset, sizeof(pj_fd_set_t));
    pj_memcpy(&wfdset, &ioqueue->wfdset, sizeof(pj_fd_set_t));
#if PJ_HAS_TCP
    pj_memcpy(&xfdset, &ioqueue->xfdset, sizeof(pj_fd_set_t));
#else
    PJ_FD_ZERO(&xfdset);
#endif

#if VALIDATE_FD_SET
    validate_sets(ioqueue, &rfdset, &wfdset, &xfdset);
#endif

    nfds = ioqueue->nfds;

    /* 在select（）之前解锁ioqueue */
    pj_lock_release(ioqueue->lock);

#if defined(PJ_WIN32_WINPHONE8) && PJ_WIN32_WINPHONE8
    count = 0;
    __try {
#endif

    count = pj_sock_select(nfds + 1, &rfdset, &wfdset, &xfdset,
                           timeout);

#if defined(PJ_WIN32_WINPHONE8) && PJ_WIN32_WINPHONE8
    /* 忽略select()引发的无效句柄异常 */
    }
    __except (GetExceptionCode() == STATUS_INVALID_HANDLE ?
          EXCEPTION_CONTINUE_EXECUTION : EXCEPTION_CONTINUE_SEARCH) {
    }
#endif

    if (count == 0)
        return 0;
    else if (count < 0)
        return -pj_get_netos_error();

    /* 扫描事件的描述符集，并将事件添加到事件数组中，以便稍后在此函数中处理。我们这样做是为了在不持有ioqueue锁的情况下并行处理事件。
     */
    pj_lock_acquire(ioqueue->lock);

    event_cnt = 0;

    /* 首先扫描可写套接字以处理accept()附带的piggy-back 数据。
     */
    for (h = ioqueue->active_list.next;
         h != &ioqueue->active_list && event_cnt < MAX_EVENTS;
         h = h->next) {
        if (h->fd == PJ_INVALID_SOCKET)
            continue;

        if ((key_has_pending_write(h) || key_has_pending_connect(h))
            && PJ_FD_ISSET(h->fd, &wfdset) && !IS_CLOSING(h)) {
#if PJ_IOQUEUE_HAS_SAFE_UNREG
            increment_counter(h);
#endif
            event[event_cnt].key = h;
            event[event_cnt].event_type = WRITEABLE_EVENT;
            ++event_cnt;
        }

        /* 扫描可读的套接字 */
        if ((key_has_pending_read(h) || key_has_pending_accept(h))
            && PJ_FD_ISSET(h->fd, &rfdset) && !IS_CLOSING(h) &&
            event_cnt < MAX_EVENTS) {
#if PJ_IOQUEUE_HAS_SAFE_UNREG
            increment_counter(h);
#endif
            event[event_cnt].key = h;
            event[event_cnt].event_type = READABLE_EVENT;
            ++event_cnt;
        }

#if PJ_HAS_TCP
        if (key_has_pending_connect(h) && PJ_FD_ISSET(h->fd, &xfdset) &&
            !IS_CLOSING(h) && event_cnt < MAX_EVENTS) {
#if PJ_IOQUEUE_HAS_SAFE_UNREG
            increment_counter(h);
#endif
            event[event_cnt].key = h;
            event[event_cnt].event_type = EXCEPTION_EVENT;
            ++event_cnt;
        }
#endif
    }

    for (i = 0; i < event_cnt; ++i) {
        if (event[i].key->grp_lock)
            pj_grp_lock_add_ref_dbg(event[i].key->grp_lock, "ioqueue", 0);
    }

    PJ_RACE_ME(5);

    pj_lock_release(ioqueue->lock);

    PJ_RACE_ME(5);

    processed_cnt = 0;

    /* 现在处理所有事件。调度功能将负责锁定每个钥匙
     */
    for (i = 0; i < event_cnt; ++i) {

        /* 只需在单个轮询中不超过PJ_IOQUEUE_MAX_EVENTS_IN_SINGLE_POLL即可 */
        if (processed_cnt < PJ_IOQUEUE_MAX_EVENTS_IN_SINGLE_POLL) {
            switch (event[i].event_type) {
                case READABLE_EVENT:
                    if (ioqueue_dispatch_read_event(ioqueue, event[i].key))
                        ++processed_cnt;
                    break;
                case WRITEABLE_EVENT:
                    if (ioqueue_dispatch_write_event(ioqueue, event[i].key))
                        ++processed_cnt;
                    break;
                case EXCEPTION_EVENT:
                    if (ioqueue_dispatch_exception_event(ioqueue, event[i].key))
                        ++processed_cnt;
                    break;
                case NO_EVENT:
                    pj_assert(!"Invalid event!");
                    break;
            }
        }

#if PJ_IOQUEUE_HAS_SAFE_UNREG
        decrement_counter(event[i].key);
#endif

        if (event[i].key->grp_lock)
            pj_grp_lock_dec_ref_dbg(event[i].key->grp_lock,
                                    "ioqueue", 0);
    }

    TRACE__((THIS_FILE, "     poll: count=%d events=%d processed=%d",
            count, event_cnt, processed_cnt));

    return processed_cnt;
}

