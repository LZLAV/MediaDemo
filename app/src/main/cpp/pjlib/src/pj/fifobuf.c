/**
 * 已完成：
 *      封装指定buf 为 fifobuf
 *
 *      分配 SZ 大小内存
 *      回收 SZ 大小内存
 *
 *      销毁 fifobuf
 *
 */
#include <pj/fifobuf.h>
#include <pj/log.h>
#include <pj/assert.h>
#include <pj/os.h>

#define THIS_FILE   "fifobuf"

#define SZ  sizeof(unsigned)

PJ_DEF(void) pj_fifobuf_init(pj_fifobuf_t *fifobuf, void *buffer, unsigned size) {
    PJ_CHECK_STACK();

    PJ_LOG(6, (THIS_FILE,
            "fifobuf_init fifobuf=%p buffer=%p, size=%d",
            fifobuf, buffer, size));

    fifobuf->first = (char *) buffer;
    fifobuf->last = fifobuf->first + size;
    fifobuf->ubegin = fifobuf->uend = fifobuf->first;
    fifobuf->full = 0;
}

/**
 * 当前 fifo buf  最大可用长度
 * @param fifobuf
 * @return
 */
PJ_DEF(unsigned) pj_fifobuf_max_size(pj_fifobuf_t *fifobuf) {
    unsigned s1, s2;

    PJ_CHECK_STACK();

    if (fifobuf->uend >= fifobuf->ubegin) {
        s1 = (unsigned) (fifobuf->last - fifobuf->uend);
        s2 = (unsigned) (fifobuf->ubegin - fifobuf->first);
    } else {
        s1 = s2 = (unsigned) (fifobuf->ubegin - fifobuf->uend);
    }

    return s1 < s2 ? s2 : s1;
}

/**
 * 分配 SZ  的内存
 * @param fifobuf
 * @param size
 * @return
 */
PJ_DEF(void*)pj_fifobuf_alloc(pj_fifobuf_t *fifobuf, unsigned size) {
    unsigned available;
    char *start;

    PJ_CHECK_STACK();

    if (fifobuf->full) {
        PJ_LOG(6, (THIS_FILE,
                "fifobuf_alloc fifobuf=%p, size=%d: full!",
                fifobuf, size));
        return NULL;
    }

    /* 尝试从fifo的结尾部分进行分配 */
    if (fifobuf->uend >= fifobuf->ubegin) {
        available = (unsigned) (fifobuf->last - fifobuf->uend);
        if (available >= size + SZ) {
            char *ptr = fifobuf->uend;
            fifobuf->uend += (size + SZ);
            if (fifobuf->uend == fifobuf->last)
                fifobuf->uend = fifobuf->first;
            if (fifobuf->uend == fifobuf->ubegin)
                fifobuf->full = 1;
            *(unsigned *) ptr = size + SZ;
            ptr += SZ;

            PJ_LOG(6, (THIS_FILE,
                    "fifobuf_alloc fifobuf=%p, size=%d: returning %p, p1=%p, p2=%p",
                    fifobuf, size, ptr, fifobuf->ubegin, fifobuf->uend));
            return ptr;
        }
    }

    /* 尝试从fifo的开始部分进行分配 */
    start = (fifobuf->uend <= fifobuf->ubegin) ? fifobuf->uend : fifobuf->first;
    available = (unsigned) (fifobuf->ubegin - start);
    if (available >= size + SZ) {
        char *ptr = start;
        fifobuf->uend = start + size + SZ;
        if (fifobuf->uend == fifobuf->ubegin)
            fifobuf->full = 1;
        *(unsigned *) ptr = size + SZ;
        ptr += SZ;

        PJ_LOG(6, (THIS_FILE,
                "fifobuf_alloc fifobuf=%p, size=%d: returning %p, p1=%p, p2=%p",
                fifobuf, size, ptr, fifobuf->ubegin, fifobuf->uend));
        return ptr;
    }

    PJ_LOG(6, (THIS_FILE,
            "fifobuf_alloc fifobuf=%p, size=%d: no space left! p1=%p, p2=%p",
            fifobuf, size, fifobuf->ubegin, fifobuf->uend));
    return NULL;
}
/**
 * 释放 SZ 的内存
 * @param fifobuf
 * @param buf
 * @return
 */
PJ_DEF(pj_status_t) pj_fifobuf_unalloc(pj_fifobuf_t *fifobuf, void *buf) {
    char *ptr = (char *) buf;
    char *endptr;
    unsigned sz;

    PJ_CHECK_STACK();

    ptr -= SZ;
    sz = *(unsigned *) ptr;

    endptr = fifobuf->uend;
    if (endptr == fifobuf->first)
        endptr = fifobuf->last;

    if (ptr + sz != endptr) {
        pj_assert(!"Invalid pointer to undo alloc");
        return -1;
    }

    fifobuf->uend = ptr;
    fifobuf->full = 0;

    PJ_LOG(6, (THIS_FILE,
            "fifobuf_unalloc fifobuf=%p, ptr=%p, size=%d, p1=%p, p2=%p",
            fifobuf, buf, sz, fifobuf->ubegin, fifobuf->uend));

    return 0;
}
/**
 *
 * @param fifobuf
 * @param buf
 * @return
 */
PJ_DEF(pj_status_t) pj_fifobuf_free(pj_fifobuf_t *fifobuf, void *buf) {
    char *ptr = (char *) buf;
    char *end;
    unsigned sz;

    PJ_CHECK_STACK();

    ptr -= SZ;
    if (ptr < fifobuf->first || ptr >= fifobuf->last) {
        pj_assert(!"Invalid pointer to free");
        return -1;
    }

    if (ptr != fifobuf->ubegin && ptr != fifobuf->first) {
        pj_assert(!"Invalid free() sequence!");
        return -1;
    }

    end = (fifobuf->uend > fifobuf->ubegin) ? fifobuf->uend : fifobuf->last;
    sz = *(unsigned *) ptr;
    if (ptr + sz > end) {
        pj_assert(!"Invalid size!");
        return -1;
    }

    fifobuf->ubegin = ptr + sz;

    /* 翻转 */
    if (fifobuf->ubegin == fifobuf->last)
        fifobuf->ubegin = fifobuf->first;

    /* 如果fifobuf为空，则重置 */
    if (fifobuf->ubegin == fifobuf->uend)
        fifobuf->ubegin = fifobuf->uend = fifobuf->first;

    fifobuf->full = 0;

    PJ_LOG(6, (THIS_FILE,
            "fifobuf_free fifobuf=%p, ptr=%p, size=%d, p1=%p, p2=%p",
            fifobuf, buf, sz, fifobuf->ubegin, fifobuf->uend));

    return 0;
}
