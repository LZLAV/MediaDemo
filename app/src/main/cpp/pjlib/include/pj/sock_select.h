/**
 * 已完成：
 * 		select
 * 			fdset 管理
 * 			select 抽象
 */
#ifndef __PJ_SELECT_H__
#define __PJ_SELECT_H__

/**
 * @file sock_select.h
 * @brief Socket select().
 */

#include <pj/types.h>

PJ_BEGIN_DECL 

/**
 * @defgroup PJ_SOCK_SELECT Socket select() API.
 * @ingroup PJ_IO
 * @{
 * 此模块为类似 select() 的API提供可移植的抽象。需要抽象，以便它可以利用跨平台可用的各种事件调度机制。
 *
 * API与正常的 select()用法非常相似。
 *
 * \section pj_sock_select_examples_sec 例子
 *
 * 有关如何使用select API的一些示例，请参见：
 *
 *  - \ref page_pjlib_select_test
 */

/**
 * pj_fd_set 是可移植结构声明。pj_sock_select()的实现本身并不使用此结构，而是使用本机 fd_set 结构。但是，
 * 我们必须确保 pj_fd_set_t 的大小能够适应本地 fd_set 结构
 */
typedef struct pj_fd_set_t
{
    pj_sock_t data[PJ_IOQUEUE_MAX_HANDLES+ 4]; /**< fd_set 不透明的缓存区*/
} pj_fd_set_t;


/**
 * 将fdsetp初始化为空集
 *
 * @param fdsetp    集合描述符
 */
PJ_DECL(void) PJ_FD_ZERO(pj_fd_set_t *fdsetp);


/**
 * 这是一个内部函数，应用程序不应该使用它。
 * 获取集合中描述符的数目。这是在 sock_select.c 中定义的。此函数仅返回 PJ_FD_SET 操作中设置的套接字数。
 * 当通过其他方式（如select()）修改集合时，计数将不会反映在这里。
 *
 * @param fdsetp    集合描述符
 *
 * @return          集合描述符的数目
 */
PJ_DECL(pj_size_t) PJ_FD_COUNT(const pj_fd_set_t *fdsetp);


/**
 * 将文件描述符 fd 添加到 fdsetp 指向的集合中。如果文件描述符fd已经在这个集合中，则对集合没有影响，也不会返回错误。
 *
 * @param fd	    socket 描述符
 * @param fdsetp    The descriptor set.
 */
PJ_DECL(void) PJ_FD_SET(pj_sock_t fd, pj_fd_set_t *fdsetp);

/**
 * 从fdsetp指向的集合中删除文件描述符fd。如果fd不是此集合的成员，则不会对集合产生任何影响，也不会返回错误。
 *
 * @param fd	    The socket descriptor.
 * @param fdsetp    The descriptor set.
 */
PJ_DECL(void) PJ_FD_CLR(pj_sock_t fd, pj_fd_set_t *fdsetp);


/**
 * 如果文件描述符fd是fdsetp指向的集合的成员，
 * 判断 fd 是否是 fdsetp 集合中成员，是则返回非零，否则返回 0
 *
 * @param fd	    The socket descriptor.
 * @param fdsetp    The descriptor set.
 *
 * @return	    是则返回非零，否则返回 0
 */
PJ_DECL(pj_bool_t) PJ_FD_ISSET(pj_sock_t fd, const pj_fd_set_t *fdsetp);


/**
 * 此函数将等待多个文件描述符更改状态。其行为与标准 bsd 套接字库中出现的 select() 函数调用相同。
 *
 * @param n		在 unice 上，这指定三个集合中的任意一个集合中编号最高的描述符，加1。在Windows上，该值将被忽略。
 * @param readfds   指向要检查可读性的一组套接字的可选指针
 * @param writefds  指向要检查可写性的一组套接字的可选指针
 * @param exceptfds 指向要检查错误的一组套接字的可选指针
 * @param timeout   选择等待的最大时间，或阻塞操作则为空
 *
 * @return	    准备好的套接字句柄总数，超时返回0，发生错误返回 -1
 */
PJ_DECL(int) pj_sock_select( int n, 
			     pj_fd_set_t *readfds, 
			     pj_fd_set_t *writefds,
			     pj_fd_set_t *exceptfds, 
			     const pj_time_val *timeout);


/**
 * @}
 */


PJ_END_DECL

#endif	/* __PJ_SELECT_H__ */
