/**
 * 已完成：
 *      select 函数操作
 *
 *      系统宏定义：
 *          FD_ZERO()       //初始化为0
 *          FD_SET()        //设置，添加
 *          FD_CLR()        //清除
 *          FD_ISSET()      //判断
 *
 */
#include <pj/sock_select.h>
#include <pj/compat/socket.h>
#include <pj/os.h>
#include <pj/assert.h>
#include <pj/errno.h>

#if defined(PJ_HAS_STRING_H) && PJ_HAS_STRING_H != 0

#   include <string.h>

#endif

#if defined(PJ_HAS_SYS_TIME_H) && PJ_HAS_SYS_TIME_H != 0
#   include <sys/time.h>
#endif

#ifdef _MSC_VER
#   pragma warning(disable: 4018)    // Signed/unsigned mismatch in FD_*
#   pragma warning(disable: 4389)    // Signed/unsigned mismatch in FD_*
#endif

#define PART_FDSET(ps)        ((fd_set*)&ps->data[1])
#define PART_FDSET_OR_NULL(ps)    (ps ? PART_FDSET(ps) : NULL)
#define PART_COUNT(ps)        (ps->data[0])

PJ_DEF(void) PJ_FD_ZERO(pj_fd_set_t *fdsetp) {
    PJ_CHECK_STACK();
    pj_assert(sizeof(pj_fd_set_t) - sizeof(pj_sock_t) >= sizeof(fd_set));

    FD_ZERO(PART_FDSET(fdsetp));
    PART_COUNT(fdsetp) = 0;
}


PJ_DEF(void) PJ_FD_SET(pj_sock_t fd, pj_fd_set_t *fdsetp) {
    PJ_CHECK_STACK();
    pj_assert(sizeof(pj_fd_set_t) - sizeof(pj_sock_t) >= sizeof(fd_set));

    if (!PJ_FD_ISSET(fd, fdsetp))
        ++PART_COUNT(fdsetp);
    FD_SET(fd, PART_FDSET(fdsetp));
}


PJ_DEF(void) PJ_FD_CLR(pj_sock_t fd, pj_fd_set_t *fdsetp) {
    PJ_CHECK_STACK();
    pj_assert(sizeof(pj_fd_set_t) - sizeof(pj_sock_t) >= sizeof(fd_set));

    if (PJ_FD_ISSET(fd, fdsetp))
        --PART_COUNT(fdsetp);
    FD_CLR(fd, PART_FDSET(fdsetp));
}


PJ_DEF(pj_bool_t) PJ_FD_ISSET(pj_sock_t fd, const pj_fd_set_t *fdsetp) {
    PJ_CHECK_STACK();
    PJ_ASSERT_RETURN(sizeof(pj_fd_set_t) - sizeof(pj_sock_t) >= sizeof(fd_set),
                     0);

    return FD_ISSET(fd, PART_FDSET(fdsetp));
}

PJ_DEF(pj_size_t) PJ_FD_COUNT(const pj_fd_set_t *fdsetp) {
    return PART_COUNT(fdsetp);
}

PJ_DEF(int) pj_sock_select(int n,
                           pj_fd_set_t *readfds,
                           pj_fd_set_t *writefds,
                           pj_fd_set_t *exceptfds,
                           const pj_time_val *timeout) {
    struct timeval os_timeout, *p_os_timeout;

    PJ_CHECK_STACK();

    PJ_ASSERT_RETURN(sizeof(pj_fd_set_t) - sizeof(pj_sock_t) >= sizeof(fd_set),
                     PJ_EBUG);

    if (timeout) {
        os_timeout.tv_sec = timeout->sec;
        os_timeout.tv_usec = timeout->msec * 1000;
        p_os_timeout = &os_timeout;
    } else {
        p_os_timeout = NULL;
    }

    return select(n, PART_FDSET_OR_NULL(readfds), PART_FDSET_OR_NULL(writefds),
                  PART_FDSET_OR_NULL(exceptfds), p_os_timeout);
}

