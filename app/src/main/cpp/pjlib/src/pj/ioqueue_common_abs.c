/** 已完成，ioqueue 对 socket 的封装
 */

/*
 * ioqueue_common_abs.c
 *
 * 它包含用各种事件调度机制（例如select、epoll）模拟 proactor 模式的常见功能。
 *
 * 此文件将包含在相应的ioqueue实现中。
 * 此文件不应编译为独立源。
 */

#define PENDING_RETRY    2

static void ioqueue_init(pj_ioqueue_t *ioqueue) {
    ioqueue->lock = NULL;
    ioqueue->auto_delete_lock = 0;
    ioqueue->default_concurrency = PJ_IOQUEUE_DEFAULT_ALLOW_CONCURRENCY;
}

static pj_status_t ioqueue_destroy(pj_ioqueue_t *ioqueue) {
    if (ioqueue->auto_delete_lock && ioqueue->lock) {
        pj_lock_release(ioqueue->lock);
        return pj_lock_destroy(ioqueue->lock);
    }

    return PJ_SUCCESS;
}

/*
 * pj_ioqueue_set_lock()
 */
PJ_DEF(pj_status_t) pj_ioqueue_set_lock(pj_ioqueue_t *ioqueue,
                                        pj_lock_t *lock,
                                        pj_bool_t auto_delete) {
    PJ_ASSERT_RETURN(ioqueue && lock, PJ_EINVAL);

    if (ioqueue->auto_delete_lock && ioqueue->lock) {
        pj_lock_destroy(ioqueue->lock);
    }

    ioqueue->lock = lock;
    ioqueue->auto_delete_lock = auto_delete;

    return PJ_SUCCESS;
}

static pj_status_t ioqueue_init_key(pj_pool_t *pool,
                                    pj_ioqueue_t *ioqueue,
                                    pj_ioqueue_key_t *key,
                                    pj_sock_t sock,
                                    pj_grp_lock_t *grp_lock,
                                    void *user_data,
                                    const pj_ioqueue_callback *cb) {
    pj_status_t rc;
    int optlen;

    PJ_UNUSED_ARG(pool);

    key->ioqueue = ioqueue;
    key->fd = sock;
    key->user_data = user_data;
    pj_list_init(&key->read_list);
    pj_list_init(&key->write_list);
#if PJ_HAS_TCP
    pj_list_init(&key->accept_list);
    key->connecting = 0;
#endif

    /* Save callback. */
    pj_memcpy(&key->cb, cb, sizeof(pj_ioqueue_callback));

#if PJ_IOQUEUE_HAS_SAFE_UNREG
    /* Set initial reference count to 1 */
    pj_assert(key->ref_count == 0);
    ++key->ref_count;

    key->closing = 0;
#endif

    rc = pj_ioqueue_set_concurrency(key, ioqueue->default_concurrency);
    if (rc != PJ_SUCCESS)
        return rc;

    /* 获取套接字类型。当socket类型为 datagram 时，发送期间将执行一些优化以允许并行发送操作。
     */
    optlen = sizeof(key->fd_type);
    rc = pj_sock_getsockopt(sock, pj_SOL_SOCKET(), pj_SO_TYPE(),
                            &key->fd_type, &optlen);
    if (rc != PJ_SUCCESS)
        key->fd_type = pj_SOCK_STREAM();

    /* 创建互斥锁 */
#if !PJ_IOQUEUE_HAS_SAFE_UNREG
    rc = pj_lock_create_simple_mutex(pool, NULL, &key->lock);
    if (rc != PJ_SUCCESS)
    return rc;
#endif

    /* 组锁 */
    key->grp_lock = grp_lock;
    if (key->grp_lock) {
        pj_grp_lock_add_ref_dbg(key->grp_lock, "ioqueue", 0);
    }

    return PJ_SUCCESS;
}

/*
 * pj_ioqueue_get_user_data()
 *
 * Obtain value associated with a key.
 */
PJ_DEF(void*)pj_ioqueue_get_user_data(pj_ioqueue_key_t *key) {
    PJ_ASSERT_RETURN(key != NULL, NULL);
    return key->user_data;
}

/*
 * pj_ioqueue_set_user_data()
 */
PJ_DEF(pj_status_t) pj_ioqueue_set_user_data(pj_ioqueue_key_t *key,
                                             void *user_data,
                                             void **old_data) {
    PJ_ASSERT_RETURN(key, PJ_EINVAL);

    if (old_data)
        *old_data = key->user_data;
    key->user_data = user_data;

    return PJ_SUCCESS;
}

PJ_INLINE(int) key_has_pending_write(pj_ioqueue_key_t *key) {
    return !pj_list_empty(&key->write_list);
}

PJ_INLINE(int) key_has_pending_read(pj_ioqueue_key_t *key) {
    return !pj_list_empty(&key->read_list);
}

PJ_INLINE(int) key_has_pending_accept(pj_ioqueue_key_t *key) {
#if PJ_HAS_TCP
    return !pj_list_empty(&key->accept_list);
#else
    PJ_UNUSED_ARG(key);
    return 0;
#endif
}

PJ_INLINE(int) key_has_pending_connect(pj_ioqueue_key_t *key) {
    return key->connecting;
}


#if PJ_IOQUEUE_HAS_SAFE_UNREG
#   define IS_CLOSING(key)  (key->closing)
#else
#   define IS_CLOSING(key)  (0)
#endif


/*
 * ioqueue_dispatch_event()
 *
 * 报告框架要处理的密钥中发生的事件。
 */
pj_bool_t ioqueue_dispatch_write_event(pj_ioqueue_t *ioqueue,
                                       pj_ioqueue_key_t *h) {
    pj_status_t rc;

    /* 尝试加锁 */
    rc = pj_ioqueue_trylock_key(h);
    if (rc != PJ_SUCCESS) {
        return PJ_FALSE;
    }

    if (IS_CLOSING(h)) {
        pj_ioqueue_unlock_key(h);
        return PJ_TRUE;
    }

#if defined(PJ_HAS_TCP) && PJ_HAS_TCP != 0
    if (h->connecting) {
        /*完成 connect() 操作 */
        pj_status_t status;
        pj_bool_t has_lock;

        /* Clear operation. */
        h->connecting = 0;

        ioqueue_remove_from_set(ioqueue, h, WRITEABLE_EVENT);
        ioqueue_remove_from_set(ioqueue, h, EXCEPTION_EVENT);


#if (defined(PJ_HAS_SO_ERROR) && PJ_HAS_SO_ERROR != 0)
        /* 从连接（2）：
         *  在Linux上，使用getsockopt读取SOL_SOCKET级别的SO_ERROR选项，以确定connect（）是否成功完成（如果SO_ERROR为零）。
         */
        {
            int value;
            int vallen = sizeof(value);
            int gs_rc = pj_sock_getsockopt(h->fd, SOL_SOCKET, SO_ERROR,
                                           &value, &vallen);
            if (gs_rc != 0) {
                /*
                 * 啊！！现在该怎么办？？？
                 * 只需指出 socket 已连接。一旦应用程序尝试使用套接字发送/接收，它就会出错。
                 */
                status = PJ_SUCCESS;
            } else {
                status = PJ_STATUS_FROM_OS(value);
            }
        }
#elif (defined(PJ_WIN32) && PJ_WIN32 != 0) || (defined(PJ_WIN64) && PJ_WIN64 != 0)
        status = PJ_SUCCESS; /* success */
#else
        /* Excellent information in D.J. Bernstein page:
         * http://cr.yp.to/docs/connect.html
         *
         * 似乎检测 connect() 失败的最便捷的方法是调用getpeername()。如果连接了套接字，getpeername()将返回0。
         * 如果套接字没有连接，它将返回ENOTCONN，read(fd, &ch, 1) 将通过错误滑动产生正确的 errno。这是来自 Douglas C. Schmidt and Ken Keys
         */
        {
            struct sockaddr_in addr;
            int addrlen = sizeof(addr);

            status = pj_sock_getpeername(h->fd, (struct sockaddr*)&addr,
                             &addrlen);
        }
#endif

        /* 解锁；从这一点来说，我们不需要保持键的互斥（除非禁用了并发，在这种情况下，我们应该在调用回调时保持互斥）*/
        if (h->allow_concurrent) {
            /* 当我们在回调中时，并发性可能会改变，所以将它保存到一个标志中
             */
            has_lock = PJ_FALSE;
            pj_ioqueue_unlock_key(h);
        } else {
            has_lock = PJ_TRUE;
        }

        /* Call callback. */
        if (h->cb.on_connect_complete && !IS_CLOSING(h))
            (*h->cb.on_connect_complete)(h, status);

        /* 持有锁 */
        if (has_lock) {
            pj_ioqueue_unlock_key(h);
        }

        /* Done. */

    } else
#endif /* PJ_HAS_TCP */
    if (key_has_pending_write(h)) {
        /* Socket可写 */
        struct write_operation *write_op;
        pj_ssize_t sent;
        pj_status_t send_rc = PJ_SUCCESS;

        /* 获取队列的第一个 */
        write_op = h->write_list.next;

        /* 对于数据报，我们可以从列表中删除 write_op，以便 send() 可以并行工作
         */
        if (h->fd_type == pj_SOCK_DGRAM()) {
            pj_list_erase(write_op);

            if (pj_list_empty(&h->write_list))
                ioqueue_remove_from_set(ioqueue, h, WRITEABLE_EVENT);

        }

        /*
         * 发送数据。不幸的是，我们必须在保持键的互斥时执行此操作，从而防止在单个键上并行写入
         */
        sent = write_op->size - write_op->written;
        if (write_op->op == PJ_IOQUEUE_OP_SEND) {
            send_rc = pj_sock_send(h->fd, write_op->buf + write_op->written,
                                   &sent, write_op->flags);
            /* 不能这么做。我们只有在发送完整个缓冲区后才能清除“op”
             */
            //write_op->op = 0;
        } else if (write_op->op == PJ_IOQUEUE_OP_SEND_TO) {
            int retry = 2;
            while (--retry >= 0) {
                send_rc = pj_sock_sendto(h->fd,
                                         write_op->buf + write_op->written,
                                         &sent, write_op->flags,
                                         &write_op->rmt_addr,
                                         write_op->rmt_addrlen);
#if defined(PJ_IPHONE_OS_HAS_MULTITASKING_SUPPORT) && \
        PJ_IPHONE_OS_HAS_MULTITASKING_SUPPORT != 0
                /* 这里对销毁 UDP socket 的特殊处理，见#1107 */
                if (send_rc==PJ_STATUS_FROM_OS(EPIPE) && !IS_CLOSING(h) &&
                    h->fd_type==pj_SOCK_DGRAM())
                {
                    PJ_PERROR(4,(THIS_FILE, send_rc,
                         "Send error for socket %d, retrying",
                         h->fd));
                    send_rc = replace_udp_sock(h);
                    continue;
                }
#endif
                break;
            }

            /* 不能这么做。我们只有在发送完整个缓冲区后才能清除“op”
             */
            //write_op->op = 0;
        } else {
            pj_assert(!"Invalid operation type!");
            write_op->op = PJ_IOQUEUE_OP_NONE;
            send_rc = PJ_EBUG;
        }

        if (send_rc == PJ_SUCCESS) {
            write_op->written += sent;
        } else {
            pj_assert(send_rc > 0);
            write_op->written = -send_rc;
        }

        /* 缓冲区用完了吗？ */
        if (send_rc != PJ_SUCCESS ||
            write_op->written == (pj_ssize_t) write_op->size ||
            h->fd_type == pj_SOCK_DGRAM()) {
            pj_bool_t has_lock;

            write_op->op = PJ_IOQUEUE_OP_NONE;

            if (h->fd_type != pj_SOCK_DGRAM()) {
                /* 整个流的写入完成 */
                pj_list_erase(write_op);

                /* 如果没有更多数据要发送，请清除操作 */
                if (pj_list_empty(&h->write_list))
                    ioqueue_remove_from_set(ioqueue, h, WRITEABLE_EVENT);

            }

            /* 解锁；从这一点来说，我们不需要保持键的互斥（除非禁用了并发，在这种情况下，我们应该在调用回调时保持互斥）*/
            if (h->allow_concurrent) {
                /* 当我们在回调中时，并发性可能会改变，所以将它保存到一个标志中
                 */
                has_lock = PJ_FALSE;
                pj_ioqueue_unlock_key(h);
                PJ_RACE_ME(5);
            } else {
                has_lock = PJ_TRUE;
            }

            /* Call callback. */
            if (h->cb.on_write_complete && !IS_CLOSING(h)) {
                (*h->cb.on_write_complete)(h,
                                           (pj_ioqueue_op_key_t *) write_op,
                                           write_op->written);
            }

            if (has_lock) {
                pj_ioqueue_unlock_key(h);
            }

        } else {
            pj_ioqueue_unlock_key(h);
        }

        /* Done. */
    } else {
        /*
         * 这是正常的；当为同一事件向多个线程发出信号，但最终只有一个线程能够处理该事件时，执行可能会落在这里。
         */
        pj_ioqueue_unlock_key(h);

        return PJ_FALSE;
    }

    return PJ_TRUE;
}

pj_bool_t ioqueue_dispatch_read_event(pj_ioqueue_t *ioqueue,
                                      pj_ioqueue_key_t *h) {
    pj_status_t rc;

    /* 尝试获取锁 */
    rc = pj_ioqueue_trylock_key(h);
    if (rc != PJ_SUCCESS) {
        return PJ_FALSE;
    }

    if (IS_CLOSING(h)) {
        pj_ioqueue_unlock_key(h);
        return PJ_TRUE;
    }

#   if PJ_HAS_TCP
    if (!pj_list_empty(&h->accept_list)) {

        struct accept_operation *accept_op;
        pj_bool_t has_lock;

        /* 从列表中获取一个accept操作 */
        accept_op = h->accept_list.next;
        pj_list_erase(accept_op);
        accept_op->op = PJ_IOQUEUE_OP_NONE;

        /* 如果没有更多挂起的接受，则清除fdset中的位 */
        if (pj_list_empty(&h->accept_list))
            ioqueue_remove_from_set(ioqueue, h, READABLE_EVENT);

        rc = pj_sock_accept(h->fd, accept_op->accept_fd,
                            accept_op->rmt_addr, accept_op->addrlen);
        if (rc == PJ_SUCCESS && accept_op->local_addr) {
            rc = pj_sock_getsockname(*accept_op->accept_fd,
                                     accept_op->local_addr,
                                     accept_op->addrlen);
        }

        /* 解锁；从这一点来说，我们不需要保持键的互斥（除非禁用了并发，在这种情况下，我们应该在调用回调时保持互斥）*/
        if (h->allow_concurrent) {
            /* 当我们在回调中时，并发性可能会改变，所以将它保存到一个标志中
             */
            has_lock = PJ_FALSE;
            pj_ioqueue_unlock_key(h);
            PJ_RACE_ME(5);
        } else {
            has_lock = PJ_TRUE;
        }

        /* Call callback. */
        if (h->cb.on_accept_complete && !IS_CLOSING(h)) {
            (*h->cb.on_accept_complete)(h,
                                        (pj_ioqueue_op_key_t *) accept_op,
                                        *accept_op->accept_fd, rc);
        }

        if (has_lock) {
            pj_ioqueue_unlock_key(h);
        }
    } else
#   endif
    if (key_has_pending_read(h)) {
        struct read_operation *read_op;
        pj_ssize_t bytes_read;
        pj_bool_t has_lock;

        /* 从列表中获取一个挂起的读取操作。 */
        read_op = h->read_list.next;
        pj_list_erase(read_op);

        /* 如果没有挂起的读取，则清除 fdset。 */
        if (pj_list_empty(&h->read_list))
            ioqueue_remove_from_set(ioqueue, h, READABLE_EVENT);

        bytes_read = read_op->size;

        if (read_op->op == PJ_IOQUEUE_OP_RECV_FROM) {
            read_op->op = PJ_IOQUEUE_OP_NONE;
            rc = pj_sock_recvfrom(h->fd, read_op->buf, &bytes_read,
                                  read_op->flags,
                                  read_op->rmt_addr,
                                  read_op->rmt_addrlen);
        } else if (read_op->op == PJ_IOQUEUE_OP_RECV) {
            read_op->op = PJ_IOQUEUE_OP_NONE;
            rc = pj_sock_recv(h->fd, read_op->buf, &bytes_read,
                              read_op->flags);
        } else {
            pj_assert(read_op->op == PJ_IOQUEUE_OP_READ);
            read_op->op = PJ_IOQUEUE_OP_NONE;
            /*
             * 用户指定了pj_ioqueue_read()
             * 在Win32上，我们应该执行 ReadFile()。但由于我们是因为 select() 而来到这里的，所以用户必须在h->fd上放置一个套接字描述符，
             * 在这种情况下，我们可以调用pj_sock_recv() 而不是ReadFile（）。
             *
             * 在Unix上，用户可能会将一个文件放在 h->fd中，所以我们必须在这里调用read()。
             * 这可能无法在没有read()的系统上编译。这就是为什么我们在这里只指定PJ_LINUX，以便更容易捕捉错误。
             */
#	    if defined(PJ_WIN32) && PJ_WIN32 != 0 || \
           defined(PJ_WIN64) && PJ_WIN64 != 0 || \
           defined(PJ_WIN32_WINCE) && PJ_WIN32_WINCE != 0
            rc = pj_sock_recv(h->fd, read_op->buf, &bytes_read,
              read_op->flags);
            //rc = ReadFile((HANDLE)h->fd, read_op->buf, read_op->size,
            //              &bytes_read, NULL);
#           elif (defined(PJ_HAS_UNISTD_H) && PJ_HAS_UNISTD_H != 0)
            bytes_read = read(h->fd, read_op->buf, bytes_read);
            rc = (bytes_read >= 0) ? PJ_SUCCESS : pj_get_os_error();
#           else
#               error "Implement read() for this platform!"
#           endif
        }

        if (rc != PJ_SUCCESS) {
#	    if (defined(PJ_WIN32) && PJ_WIN32 != 0) || \
           (defined(PJ_WIN64) && PJ_WIN64 != 0)
            /* On Win32, for UDP, WSAECONNRESET on the receive side
             * indicates that previous sending has triggered ICMP Port
             * Unreachable message.
             * But we wouldn't know at this point which one of previous
             * key that has triggered the error, since UDP socket can
             * be shared!
             * So we'll just ignore it!
             */

            if (rc == PJ_STATUS_FROM_OS(WSAECONNRESET)) {
            //PJ_LOG(4,(THIS_FILE,
                    //          "Ignored ICMP port unreach. on key=%p", h));
            }
#	    endif

            /* 无论如何，我们都会向调用者报告 */
            bytes_read = -rc;

#if defined(PJ_IPHONE_OS_HAS_MULTITASKING_SUPPORT) && \
    PJ_IPHONE_OS_HAS_MULTITASKING_SUPPORT != 0
            /* 这里对销毁UDP socket 的特殊处理，见 #1107 */
            if (rc == PJ_STATUS_FROM_OS(ENOTCONN) && !IS_CLOSING(h) &&
            h->fd_type==pj_SOCK_DGRAM())
            {
            rc = replace_udp_sock(h);
            if (rc != PJ_SUCCESS) {
                bytes_read = -rc;
            }
            }
#endif
        }

        /* 解锁；从这一点来说，我们不需要保持键的互斥（除非禁用了并发，在这种情况下，我们应该在调用回调时保持互斥）*/
        if (h->allow_concurrent) {
            /* 当我们在回调中时，并发性可能会被更改，所以请将其保存到一个标志中
             */
            has_lock = PJ_FALSE;
            pj_ioqueue_unlock_key(h);
            PJ_RACE_ME(5);
        } else {
            has_lock = PJ_TRUE;
        }

        /* Call callback. */
        if (h->cb.on_read_complete && !IS_CLOSING(h)) {
            (*h->cb.on_read_complete)(h,
                                      (pj_ioqueue_op_key_t *) read_op,
                                      bytes_read);
        }

        if (has_lock) {
            pj_ioqueue_unlock_key(h);
        }

    } else {
        /*
         * 这是正常的；当为同一事件向多个线程发出信号，但最终只有一个线程能够处理该事件时，执行可能会落在这里。
         */
        pj_ioqueue_unlock_key(h);

        return PJ_FALSE;
    }

    return PJ_TRUE;
}


pj_bool_t ioqueue_dispatch_exception_event(pj_ioqueue_t *ioqueue,
                                           pj_ioqueue_key_t *h) {
    pj_bool_t has_lock;
    pj_status_t rc;

    /* 尝试加锁 */
    rc = pj_ioqueue_trylock_key(h);
    if (rc != PJ_SUCCESS) {
        return PJ_FALSE;
    }

    if (!h->connecting) {
        /* 可能有多个线程被唤醒，因此剩余的线程将看到 h->connecting 为零，因为它已被其他线程处理。
         */
        pj_ioqueue_unlock_key(h);
        return PJ_TRUE;
    }

    if (IS_CLOSING(h)) {
        pj_ioqueue_unlock_key(h);
        return PJ_TRUE;
    }

    /* 清除操作 */
    h->connecting = 0;

    ioqueue_remove_from_set(ioqueue, h, WRITEABLE_EVENT);
    ioqueue_remove_from_set(ioqueue, h, EXCEPTION_EVENT);

    /* 解锁；从这一点来说，我们不需要保持键的互斥（除非禁用了并发，在这种情况下，我们应该在调用回调时保持互斥）
     */
    if (h->allow_concurrent) {
        /* 当我们在回调中时，并发性可能会改变，所以将它保存到一个标志中
         */
        has_lock = PJ_FALSE;
        pj_ioqueue_unlock_key(h);
        PJ_RACE_ME(5);
    } else {
        has_lock = PJ_TRUE;
    }

    /* Call callback. */
    if (h->cb.on_connect_complete && !IS_CLOSING(h)) {
        pj_status_t status = -1;
#if (defined(PJ_HAS_SO_ERROR) && PJ_HAS_SO_ERROR != 0)
        int value;
        int vallen = sizeof(value);
        int gs_rc = pj_sock_getsockopt(h->fd, SOL_SOCKET, SO_ERROR,
                                       &value, &vallen);
        if (gs_rc == 0) {
            status = PJ_RETURN_OS_ERROR(value);
        }
#endif

        (*h->cb.on_connect_complete)(h, status);
    }

    if (has_lock) {
        pj_ioqueue_unlock_key(h);
    }

    return PJ_TRUE;
}

/*
 * pj_ioqueue_recv()
 *
 * 从socket启动异步recv()
 */
PJ_DEF(pj_status_t) pj_ioqueue_recv(pj_ioqueue_key_t *key,
                                    pj_ioqueue_op_key_t *op_key,
                                    void *buffer,
                                    pj_ssize_t *length,
                                    unsigned flags) {
    struct read_operation *read_op;

    PJ_ASSERT_RETURN(key && op_key && buffer && length, PJ_EINVAL);
    PJ_CHECK_STACK();

    /* 检查键是否正在关闭（在访问其他变量之前需要先执行此操作，因为它们可能已被销毁。见#469
     */
    if (IS_CLOSING(key))
        return PJ_ECANCELLED;

    read_op = (struct read_operation *) op_key;
    PJ_ASSERT_RETURN(read_op->op == PJ_IOQUEUE_OP_NONE, PJ_EPENDING);
    read_op->op = PJ_IOQUEUE_OP_NONE;

    /* 试着看看是否有立即可用的数据
     */
    if ((flags & PJ_IOQUEUE_ALWAYS_ASYNC) == 0) {
        pj_status_t status;
        pj_ssize_t size;

        size = *length;
        status = pj_sock_recv(key->fd, buffer, &size, flags);
        if (status == PJ_SUCCESS) {
            /* 数据可用 */
            *length = size;
            return PJ_SUCCESS;
        } else {
            /* 如果error不是 EWOULDBLOCK（或Linux上的EAGAIN），则向调用者报告错误
             */
            if (status != PJ_STATUS_FROM_OS(PJ_BLOCKING_ERROR_VAL))
                return status;
        }
    }

    flags &= ~(PJ_IOQUEUE_ALWAYS_ASYNC);

    /*
     * 没有立即可用的数据。必须将异步操作安排到 ioqueue
     */
    read_op->op = PJ_IOQUEUE_OP_RECV;
    read_op->buf = buffer;
    read_op->size = *length;
    read_op->flags = flags;

    pj_ioqueue_lock_key(key);
    /* 再检查一遍。在多线程应用程序上一次签入后，句柄可能已关闭。如果我们将坏句柄添加到集合中，它将损坏 ioqueue 集合。见#913
     */
    if (IS_CLOSING(key)) {
        pj_ioqueue_unlock_key(key);
        return PJ_ECANCELLED;
    }
    pj_list_insert_before(&key->read_list, read_op);
    ioqueue_add_to_set(key->ioqueue, key, READABLE_EVENT);
    pj_ioqueue_unlock_key(key);

    return PJ_EPENDING;
}

/*
 * pj_ioqueue_recvfrom()
 *
 * 从套接字启动异步recvfrom()
 */
PJ_DEF(pj_status_t) pj_ioqueue_recvfrom(pj_ioqueue_key_t *key,
                                        pj_ioqueue_op_key_t *op_key,
                                        void *buffer,
                                        pj_ssize_t *length,
                                        unsigned flags,
                                        pj_sockaddr_t *addr,
                                        int *addrlen) {
    struct read_operation *read_op;

    PJ_ASSERT_RETURN(key && op_key && buffer && length, PJ_EINVAL);
    PJ_CHECK_STACK();

    /* 检查key是否已关闭 */
    if (IS_CLOSING(key))
        return PJ_ECANCELLED;

    read_op = (struct read_operation *) op_key;
    if (flags == 0) {
        PJ_ASSERT_RETURN(read_op->op == PJ_IOQUEUE_OP_NONE, PJ_EPENDING);
    }
    read_op->op = PJ_IOQUEUE_OP_NONE;

    /* 试着看看是否有立即可用的数据
     */
    if ((flags & PJ_IOQUEUE_ALWAYS_ASYNC) == 0) {
        pj_status_t status;
        pj_ssize_t size;

        size = *length;
        status = pj_sock_recvfrom(key->fd, buffer, &size, flags,
                                  addr, addrlen);
        if (status == PJ_SUCCESS) {
            /* 数据可用 */
            *length = size;
            return PJ_SUCCESS;
        } else {
            /* 如果error不是EWOULDBLOCK（或Linux上的EAGAIN），则向调用者报告错误。
             */
            if (status != PJ_STATUS_FROM_OS(PJ_BLOCKING_ERROR_VAL))
                return status;
        }
    }

    flags &= ~(PJ_IOQUEUE_ALWAYS_ASYNC);

    /*
     * 没有立即可用的数据。必须将异步操作安排到 ioqueue
     */
    read_op->op = PJ_IOQUEUE_OP_RECV_FROM;
    read_op->buf = buffer;
    read_op->size = *length;
    read_op->flags = flags;
    read_op->rmt_addr = addr;
    read_op->rmt_addrlen = addrlen;

    pj_ioqueue_lock_key(key);
    /* 再检查一遍。在多线程应用程序上一次签入后，句柄可能已关闭。如果我们将坏句柄添加到集合中，它将损坏 ioqueue 集合。见#913
     */
    if (IS_CLOSING(key)) {
        pj_ioqueue_unlock_key(key);
        return PJ_ECANCELLED;
    }
    pj_list_insert_before(&key->read_list, read_op);
    ioqueue_add_to_set(key->ioqueue, key, READABLE_EVENT);
    pj_ioqueue_unlock_key(key);

    return PJ_EPENDING;
}

/*
 * pj_ioqueue_send()
 *
 * 启动描述符的异步send()
 */
PJ_DEF(pj_status_t) pj_ioqueue_send(pj_ioqueue_key_t *key,
                                    pj_ioqueue_op_key_t *op_key,
                                    const void *data,
                                    pj_ssize_t *length,
                                    unsigned flags) {
    struct write_operation *write_op;
    pj_status_t status;
    unsigned retry;
    pj_ssize_t sent;

    PJ_ASSERT_RETURN(key && op_key && data && length, PJ_EINVAL);
    PJ_CHECK_STACK();

    /* 检查 key 是否关闭 */
    if (IS_CLOSING(key))
        return PJ_ECANCELLED;

    /* 我们不能使用 PJ_IOQUEUE_ALWAYS_ASYNC 进行套接字写入 */
    flags &= ~(PJ_IOQUEUE_ALWAYS_ASYNC);

    /* 快速通道：
     *  尝试立即发送数据，只要没有挂起的写入！
     * 注：
     *  我们推测在没有正确获取ioqueue的互斥量的情况下，这里的列表是空的。这是有意的，通过并行性来最大化性能。
     *  这应该是安全的，因为：
     *      -按照约定，我们要求调用者确保在其他线程调用同一密钥上的操作时，该密钥没有被注销。
     *      -pj_list_empty() 可以由多个线程安全地调用，即使其他线程正在修改列表。
     */
    if (pj_list_empty(&key->write_list)) {
        /*
         * 看看是否可以立即发送数据。
         */
        sent = *length;
        status = pj_sock_send(key->fd, data, &sent, flags);
        if (status == PJ_SUCCESS) {
            /* Success! */
            *length = sent;
            return PJ_SUCCESS;
        } else {
            /* 如果error不是EWOULDBLOCK（或Linux上的EAGAIN），则向调用者报告错误
             */
            if (status != PJ_STATUS_FROM_OS(PJ_BLOCKING_ERROR_VAL)) {
                return status;
            }
        }
    }

    /*
     * 安排异步发送
     */
    write_op = (struct write_operation *) op_key;

    /* 如果写入操作有挂起操作，则自旋 */
    for (retry = 0; write_op->op != 0 && retry < PENDING_RETRY; ++retry)
        pj_thread_sleep(0);

    /* 最后的机会 */
    if (write_op->op) {
        /*
         * 无法发送数据包，因为写入操作中已存在挂起的写入操作。我们无法将操作放入写入操作中，
         * 因为写入操作已包含挂起的操作！我们也不能用send()直接发送数据包，因为这会破坏数据
         * 包的顺序。所以我们只能在这里返回错误。
         *
         * 这可能发生在多线程程序中，其中轮询由一个线程完成，而其他线程只执行发送。如果轮询线程
         * 运行的优先级低于发送线程，则可能未及时清除挂起写入标志，因为清除仅在轮询期间完成
         *
         * 应用程序应该在这种情况下指定多个写操作键。
         *
         */
        //pj_assert(!"ioqueue: there is pending operation on this key!");
        return PJ_EBUSY;
    }

    write_op->op = PJ_IOQUEUE_OP_SEND;
    write_op->buf = (char *) data;
    write_op->size = *length;
    write_op->written = 0;
    write_op->flags = flags;

    pj_ioqueue_lock_key(key);
    /* 再检查一遍。在多线程应用程序上一次签入后，句柄可能已关闭。如果我们将坏句柄添加到集合中，它将损坏ioqueue集合。见#913
     */
    if (IS_CLOSING(key)) {
        pj_ioqueue_unlock_key(key);
        return PJ_ECANCELLED;
    }
    pj_list_insert_before(&key->write_list, write_op);
    ioqueue_add_to_set(key->ioqueue, key, WRITEABLE_EVENT);
    pj_ioqueue_unlock_key(key);

    return PJ_EPENDING;
}


/*
 * pj_ioqueue_sendto()
 *
 * 启动描述符的异步write()
 */
PJ_DEF(pj_status_t) pj_ioqueue_sendto(pj_ioqueue_key_t *key,
                                      pj_ioqueue_op_key_t *op_key,
                                      const void *data,
                                      pj_ssize_t *length,
                                      pj_uint32_t flags,
                                      const pj_sockaddr_t *addr,
                                      int addrlen) {
    struct write_operation *write_op;
    unsigned retry;
    pj_bool_t restart_retry = PJ_FALSE;
    pj_status_t status;
    pj_ssize_t sent;

    PJ_ASSERT_RETURN(key && op_key && data && length, PJ_EINVAL);
    PJ_CHECK_STACK();

#if defined(PJ_IPHONE_OS_HAS_MULTITASKING_SUPPORT) && \
        PJ_IPHONE_OS_HAS_MULTITASKING_SUPPORT != 0
    retry_on_restart:
#else
    PJ_UNUSED_ARG(restart_retry);
#endif
    /* 检查 key 是否关闭 */
    if (IS_CLOSING(key))
        return PJ_ECANCELLED;

    /* 我们不能使用PJ_IOQUEUE_ALWAYS_ASYNC进行套接字写入 */
    flags &= ~(PJ_IOQUEUE_ALWAYS_ASYNC);

    /* 快速通道：
     *      尝试立即发送数据，除非没有挂起的写入！
     *  注：
     *      我们推测在没有正确获取 ioqueue 的互斥量的情况下，这里的列表是空的。这是有意的，通过并行性来最大化性能
     *      这应该是安全的，因为：
     *          -按照约定，我们要求调用者确保在其他线程调用同一密钥上的操作时，该密钥没有被注销
     *          -pj_list_empty() 可以由多个线程安全地调用，即使其他线程正在修改列表
     */
    if (pj_list_empty(&key->write_list)) {
        /*
         * 看看是否可以立即发送数据。
         */
        sent = *length;
        status = pj_sock_sendto(key->fd, data, &sent, flags, addr, addrlen);
        if (status == PJ_SUCCESS) {
            /* 成功 */
            *length = sent;
            return PJ_SUCCESS;
        } else {
            /* 如果error不是EWOULDBLOCK（或Linux上的EAGAIN），则向调用者报告错误
             */
            if (status != PJ_STATUS_FROM_OS(PJ_BLOCKING_ERROR_VAL)) {
#if defined(PJ_IPHONE_OS_HAS_MULTITASKING_SUPPORT) && \
        PJ_IPHONE_OS_HAS_MULTITASKING_SUPPORT != 0
                /* 这里对销毁 UDP socket 的特殊处理，见票#1107 */
                if (status==PJ_STATUS_FROM_OS(EPIPE) && !IS_CLOSING(key) &&
                    key->fd_type==pj_SOCK_DGRAM() && !restart_retry)
                {
                    PJ_PERROR(4,(THIS_FILE, status,
                         "Send error for socket %d, retrying",
                         key->fd));
                    replace_udp_sock(key);
                    restart_retry = PJ_TRUE;
                    goto retry_on_restart;
                }
#endif

                return status;
            }
        }
    }

    /*
     * 检查地址存储器是否可以保存address参数
     */
    PJ_ASSERT_RETURN(addrlen <= (int) sizeof(pj_sockaddr_in), PJ_EBUG);

    /*
     * 安排异步发送
     */
    write_op = (struct write_operation *) op_key;

    /* 如果写入操作有挂起操作，则自旋 */
    for (retry = 0; write_op->op != 0 && retry < PENDING_RETRY; ++retry)
        pj_thread_sleep(0);

    /* 最后的机会 */
    if (write_op->op) {
        /* 无法发送数据包，因为写入操作上已存在挂起的写入操作。我们无法将操作放入写入操作，
         * 因为写入操作已包含挂起的操作！我们也不能用 sendto() 直接发送数据包，因为这会破
         * 坏数据包的顺序。所以我们只能在这里返回错误。
         *
         * 这可能发生在多线程程序中，其中轮询由一个线程完成，而其他线程只执行发送。如果轮询线程
         * 运行的优先级低于发送线程，则可能未及时清除挂起写入标志，因为清除仅在轮询期间完成。
         *
         * 应用程序应该在这种情况下指定多个写操作键。
         */
        //pj_assert(!"ioqueue: there is pending operation on this key!");
        return PJ_EBUSY;
    }

    write_op->op = PJ_IOQUEUE_OP_SEND_TO;
    write_op->buf = (char *) data;
    write_op->size = *length;
    write_op->written = 0;
    write_op->flags = flags;
    pj_memcpy(&write_op->rmt_addr, addr, addrlen);
    write_op->rmt_addrlen = addrlen;

    pj_ioqueue_lock_key(key);
    /* 再检查一遍。在多线程应用程序上一次签入后，句柄可能已关闭。如果我们将坏句柄添加到集合中，它将损坏ioqueue集合。见#913
     */
    if (IS_CLOSING(key)) {
        pj_ioqueue_unlock_key(key);
        return PJ_ECANCELLED;
    }
    pj_list_insert_before(&key->write_list, write_op);
    ioqueue_add_to_set(key->ioqueue, key, WRITEABLE_EVENT);
    pj_ioqueue_unlock_key(key);

    return PJ_EPENDING;
}

#if PJ_HAS_TCP
/*
 * Initiate overlapped accept() operation.
 */
PJ_DEF(pj_status_t) pj_ioqueue_accept(pj_ioqueue_key_t *key,
                                      pj_ioqueue_op_key_t *op_key,
                                      pj_sock_t *new_sock,
                                      pj_sockaddr_t *local,
                                      pj_sockaddr_t *remote,
                                      int *addrlen) {
    struct accept_operation *accept_op;
    pj_status_t status;

    /* check parameters. All must be specified! */
    PJ_ASSERT_RETURN(key && op_key && new_sock, PJ_EINVAL);

    /* Check if key is closing. */
    if (IS_CLOSING(key))
        return PJ_ECANCELLED;

    accept_op = (struct accept_operation *) op_key;
    PJ_ASSERT_RETURN(accept_op->op == PJ_IOQUEUE_OP_NONE, PJ_EPENDING);
    accept_op->op = PJ_IOQUEUE_OP_NONE;

    /* Fast track:
     *  See if there's new connection available immediately.
     */
    if (pj_list_empty(&key->accept_list)) {
        status = pj_sock_accept(key->fd, new_sock, remote, addrlen);
        if (status == PJ_SUCCESS) {
            /* Yes! New connection is available! */
            if (local && addrlen) {
                status = pj_sock_getsockname(*new_sock, local, addrlen);
                if (status != PJ_SUCCESS) {
                    pj_sock_close(*new_sock);
                    *new_sock = PJ_INVALID_SOCKET;
                    return status;
                }
            }
            return PJ_SUCCESS;
        } else {
            /* If error is not EWOULDBLOCK (or EAGAIN on Linux), report
             * the error to caller.
             */
            if (status != PJ_STATUS_FROM_OS(PJ_BLOCKING_ERROR_VAL)) {
                return status;
            }
        }
    }

    /*
     * No connection is available immediately.
     * Schedule accept() operation to be completed when there is incoming
     * connection available.
     */
    accept_op->op = PJ_IOQUEUE_OP_ACCEPT;
    accept_op->accept_fd = new_sock;
    accept_op->rmt_addr = remote;
    accept_op->addrlen = addrlen;
    accept_op->local_addr = local;

    pj_ioqueue_lock_key(key);
    /* Check again. Handle may have been closed after the previous check
     * in multithreaded app. If we add bad handle to the set it will
     * corrupt the ioqueue set. See #913
     */
    if (IS_CLOSING(key)) {
        pj_ioqueue_unlock_key(key);
        return PJ_ECANCELLED;
    }
    pj_list_insert_before(&key->accept_list, accept_op);
    ioqueue_add_to_set(key->ioqueue, key, READABLE_EVENT);
    pj_ioqueue_unlock_key(key);

    return PJ_EPENDING;
}

/*
 * Initiate overlapped connect() operation (well, it's non-blocking actually,
 * since there's no overlapped version of connect()).
 */
PJ_DEF(pj_status_t) pj_ioqueue_connect(pj_ioqueue_key_t *key,
                                       const pj_sockaddr_t *addr,
                                       int addrlen) {
    pj_status_t status;

    /* check parameters. All must be specified! */
    PJ_ASSERT_RETURN(key && addr && addrlen, PJ_EINVAL);

    /* Check if key is closing. */
    if (IS_CLOSING(key))
        return PJ_ECANCELLED;

    /* Check if socket has not been marked for connecting */
    if (key->connecting != 0)
        return PJ_EPENDING;

    status = pj_sock_connect(key->fd, addr, addrlen);
    if (status == PJ_SUCCESS) {
        /* Connected! */
        return PJ_SUCCESS;
    } else {
        if (status == PJ_STATUS_FROM_OS(PJ_BLOCKING_CONNECT_ERROR_VAL)) {
            /* Pending! */
            pj_ioqueue_lock_key(key);
            /* Check again. Handle may have been closed after the previous
             * check in multithreaded app. See #913
             */
            if (IS_CLOSING(key)) {
                pj_ioqueue_unlock_key(key);
                return PJ_ECANCELLED;
            }
            key->connecting = PJ_TRUE;
            ioqueue_add_to_set(key->ioqueue, key, WRITEABLE_EVENT);
            ioqueue_add_to_set(key->ioqueue, key, EXCEPTION_EVENT);
            pj_ioqueue_unlock_key(key);
            return PJ_EPENDING;
        } else {
            /* Error! */
            return status;
        }
    }
}

#endif    /* PJ_HAS_TCP */


PJ_DEF(void) pj_ioqueue_op_key_init(pj_ioqueue_op_key_t *op_key,
                                    pj_size_t size) {
    pj_bzero(op_key, size);
}


/*
 * pj_ioqueue_is_pending()
 */
PJ_DEF(pj_bool_t) pj_ioqueue_is_pending(pj_ioqueue_key_t *key,
                                        pj_ioqueue_op_key_t *op_key) {
    struct generic_operation *op_rec;

    PJ_UNUSED_ARG(key);

    op_rec = (struct generic_operation *) op_key;
    return op_rec->op != 0;
}


/*
 * pj_ioqueue_post_completion()
 */
PJ_DEF(pj_status_t) pj_ioqueue_post_completion(pj_ioqueue_key_t *key,
                                               pj_ioqueue_op_key_t *op_key,
                                               pj_ssize_t bytes_status) {
    struct generic_operation *op_rec;

    /*
     * 在所有挂起的操作列表中找到操作键，以确保它仍然存在；然后调用回调
     */
    pj_ioqueue_lock_key(key);

    /* 在挂起的读取列表中查找操作 */
    op_rec = (struct generic_operation *) key->read_list.next;
    while (op_rec != (void *) &key->read_list) {
        if (op_rec == (void *) op_key) {
            pj_list_erase(op_rec);
            op_rec->op = PJ_IOQUEUE_OP_NONE;
            pj_ioqueue_unlock_key(key);

            if (key->cb.on_read_complete)
                (*key->cb.on_read_complete)(key, op_key, bytes_status);
            return PJ_SUCCESS;
        }
        op_rec = op_rec->next;
    }

    /* 在挂起的写入列表中查找操作 */
    op_rec = (struct generic_operation *) key->write_list.next;
    while (op_rec != (void *) &key->write_list) {
        if (op_rec == (void *) op_key) {
            pj_list_erase(op_rec);
            op_rec->op = PJ_IOQUEUE_OP_NONE;
            pj_ioqueue_unlock_key(key);

            if (key->cb.on_write_complete)
                (*key->cb.on_write_complete)(key, op_key, bytes_status);
            return PJ_SUCCESS;
        }
        op_rec = op_rec->next;
    }

    /* 在挂起接受列表中查找操作 */
    op_rec = (struct generic_operation *) key->accept_list.next;
    while (op_rec != (void *) &key->accept_list) {
        if (op_rec == (void *) op_key) {
            pj_list_erase(op_rec);
            op_rec->op = PJ_IOQUEUE_OP_NONE;
            pj_ioqueue_unlock_key(key);

            if (key->cb.on_accept_complete) {
                (*key->cb.on_accept_complete)(key, op_key,
                                              PJ_INVALID_SOCKET,
                                              (pj_status_t) bytes_status);
            }
            return PJ_SUCCESS;
        }
        op_rec = op_rec->next;
    }

    pj_ioqueue_unlock_key(key);

    return PJ_EINVALIDOP;
}

PJ_DEF(pj_status_t) pj_ioqueue_set_default_concurrency(pj_ioqueue_t *ioqueue,
                                                       pj_bool_t allow) {
    PJ_ASSERT_RETURN(ioqueue != NULL, PJ_EINVAL);
    ioqueue->default_concurrency = allow;
    return PJ_SUCCESS;
}


PJ_DEF(pj_status_t) pj_ioqueue_set_concurrency(pj_ioqueue_key_t *key,
                                               pj_bool_t allow) {
    PJ_ASSERT_RETURN(key, PJ_EINVAL);

    /* 如果禁用并发，则必须启用PJ_IOQUEUE_HAS_SAFE_UNREG
     */
    PJ_ASSERT_RETURN(allow || PJ_IOQUEUE_HAS_SAFE_UNREG, PJ_EINVAL);

    key->allow_concurrent = allow;
    return PJ_SUCCESS;
}

PJ_DEF(pj_status_t) pj_ioqueue_lock_key(pj_ioqueue_key_t *key) {
    if (key->grp_lock)
        return pj_grp_lock_acquire(key->grp_lock);
    else
        return pj_lock_acquire(key->lock);
}

PJ_DEF(pj_status_t) pj_ioqueue_trylock_key(pj_ioqueue_key_t *key) {
    if (key->grp_lock)
        return pj_grp_lock_tryacquire(key->grp_lock);
    else
        return pj_lock_tryacquire(key->lock);
}

PJ_DEF(pj_status_t) pj_ioqueue_unlock_key(pj_ioqueue_key_t *key) {
    if (key->grp_lock)
        return pj_grp_lock_release(key->grp_lock);
    else
        return pj_lock_release(key->lock);
}


