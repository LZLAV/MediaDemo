/**
 * 已完成
 */

/* ioqueue_common_abs.h
 *
 * 此文件包含私有声明，用于将各种事件轮询/调度机制（例如select、poll、epoll）抽象到ioqueue。
 */

#include <pj/list.h>

/*
 * select ioqueue依赖套接字函数（pj_sock_xxx()）返回正确的错误代码。
 */
#if PJ_RETURN_OS_ERROR(100) != PJ_STATUS_FROM_OS(100)
#   error "Proper error reporting must be enabled for ioqueue to work!"
#endif


struct generic_operation
{
    PJ_DECL_LIST_MEMBER(struct generic_operation);
    pj_ioqueue_operation_e  op;
};

struct read_operation
{
    PJ_DECL_LIST_MEMBER(struct read_operation);
    pj_ioqueue_operation_e  op;

    void		   *buf;
    pj_size_t		    size;
    unsigned                flags;
    pj_sockaddr_t	   *rmt_addr;
    int			   *rmt_addrlen;
};

struct write_operation
{
    PJ_DECL_LIST_MEMBER(struct write_operation);
    pj_ioqueue_operation_e  op;

    char		   *buf;
    pj_size_t		    size;
    pj_ssize_t              written;
    unsigned                flags;
    pj_sockaddr_in	    rmt_addr;
    int			    rmt_addrlen;
};

struct accept_operation
{
    PJ_DECL_LIST_MEMBER(struct accept_operation);
    pj_ioqueue_operation_e  op;

    pj_sock_t              *accept_fd;
    pj_sockaddr_t	   *local_addr;
    pj_sockaddr_t	   *rmt_addr;
    int			   *addrlen;
};

union operation_key
{
    struct generic_operation generic_op;
    struct read_operation    read;
    struct write_operation   write;
#if PJ_HAS_TCP
    struct accept_operation  accept;
#endif
};

#if PJ_IOQUEUE_HAS_SAFE_UNREG
#   define UNREG_FIELDS			\
	unsigned	    ref_count;	\
	pj_bool_t	    closing;	\
	pj_time_val	    free_time;	\
	
#else
#   define UNREG_FIELDS
#endif

#define DECLARE_COMMON_KEY                          \
    PJ_DECL_LIST_MEMBER(struct pj_ioqueue_key_t);   \
    pj_ioqueue_t           *ioqueue;                \
    pj_grp_lock_t 	   *grp_lock;		    \
    pj_lock_t              *lock;                   \
    pj_bool_t		    inside_callback;	    \
    pj_bool_t		    destroy_requested;	    \
    pj_bool_t		    allow_concurrent;	    \
    pj_sock_t		    fd;                     \
    int                     fd_type;                \
    void		   *user_data;              \
    pj_ioqueue_callback	    cb;                     \
    int                     connecting;             \
    struct read_operation   read_list;              \
    struct write_operation  write_list;             \
    struct accept_operation accept_list;	    \
    UNREG_FIELDS


#define DECLARE_COMMON_IOQUEUE                      \
    pj_lock_t          *lock;                       \
    pj_bool_t           auto_delete_lock;	    \
    pj_bool_t		default_concurrency;


enum ioqueue_event_type
{
    NO_EVENT,
    READABLE_EVENT,
    WRITEABLE_EVENT,
    EXCEPTION_EVENT,
};

static void ioqueue_add_to_set( pj_ioqueue_t *ioqueue,
                                pj_ioqueue_key_t *key,
                                enum ioqueue_event_type event_type );
static void ioqueue_remove_from_set( pj_ioqueue_t *ioqueue,
                                     pj_ioqueue_key_t *key, 
                                     enum ioqueue_event_type event_type);

