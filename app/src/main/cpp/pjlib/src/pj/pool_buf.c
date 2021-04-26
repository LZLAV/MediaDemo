/**
 * 已完成：
 *
 */
#include <pj/pool_buf.h>
#include <pj/assert.h>
#include <pj/os.h>

static struct pj_pool_factory stack_based_factory;

struct creation_param {
    void *stack_buf;
    pj_size_t size;
};

static int is_initialized;
static long tls = -1;

static void *stack_alloc(pj_pool_factory *factory, pj_size_t size);

static void pool_buf_cleanup(void) {
    if (tls != -1) {
        pj_thread_local_free(tls);
        tls = -1;
    }
    if (is_initialized)
        is_initialized = 0;
}

static pj_status_t pool_buf_initialize(void) {
    pj_atexit(&pool_buf_cleanup);

    stack_based_factory.policy.block_alloc = &stack_alloc;
    return pj_thread_local_alloc(&tls);
}

static void *stack_alloc(pj_pool_factory *factory, pj_size_t size) {
    struct creation_param *param;
    void *buf;

    PJ_UNUSED_ARG(factory);

    param = (struct creation_param *) pj_thread_local_get(tls);
    if (param == NULL) {
        /* Don't assert(), this is normal no-memory situation */
        return NULL;
    }

    pj_thread_local_set(tls, NULL);

    PJ_ASSERT_RETURN(size <= param->size, NULL);

    buf = param->stack_buf;

    /* Prevent the buffer from being reused */
    param->stack_buf = NULL;

    return buf;
}


PJ_DEF(pj_pool_t*)pj_pool_create_on_buf(const char *name,
                                        void *buf,
                                        pj_size_t size) {
#if PJ_HAS_POOL_ALT_API == 0
    struct creation_param param;
    pj_size_t align_diff;

    PJ_ASSERT_RETURN(buf && size, NULL);

    if (!is_initialized) {
        if (pool_buf_initialize() != PJ_SUCCESS)
            return NULL;
        is_initialized = 1;
    }

    /* 检查并对齐缓冲器 */
    align_diff = (pj_size_t) buf;
    if (align_diff & (PJ_POOL_ALIGNMENT - 1)) {
        align_diff &= (PJ_POOL_ALIGNMENT - 1);
        buf = (void *) (((char *) buf) + align_diff);
        size -= align_diff;
    }

    param.stack_buf = buf;
    param.size = size;
    pj_thread_local_set(tls, &param);

    return pj_pool_create_int(&stack_based_factory, name, size, 0,
                              pj_pool_factory_default_policy.callback);
#else
    PJ_UNUSED_ARG(buf);
    return pj_pool_create(NULL, name, size, size, NULL);
#endif
}

