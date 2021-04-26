/**
 *	已完成
 *		基于堆栈/缓冲区的内存池
 *
 */
#ifndef __POOL_STACK_H__
#define __POOL_STACK_H__

#include <pj/pool.h>

/**
 * @defgroup PJ_POOL_BUFFER 基于堆栈/缓冲区的内存池分配器
 * @ingroup PJ_POOL_GROUP
 * @brief 基于堆栈/缓冲区的池
 *
 * 本节描述内存池的实现，它使用从堆栈分配的内存。应用程序通过指定一个缓冲区（可以从静态内存或堆栈变量分配）来创建这个池，然后使用普通池API来访问/使用这个池。
 *
 * 如果在池创建期间指定的缓冲区是位于堆栈中的缓冲区，则当执行离开包含缓冲区的封闭块时，该池将无效（或隐式销毁）。请注意，应用程序必须确保在此池中分配的任何对象（如互斥）在该池失效之前已被销毁。
 *
 * 使用例子:
 *
 * \code
  #include <pjlib.h>

  static void test()
  {
    char buffer[500];
    pj_pool_t *pool;
    void *p;

    pool = pj_pool_create_on_buf("thepool", buffer, sizeof(buffer));

    // Use the pool as usual
    p = pj_pool_alloc(pool, ...);
    ...

    // No need to release the pool
  }

  int main()
  {
    pj_init();
    test();
    return 0;
  }

   \endcode
 *
 * @{
 */

PJ_BEGIN_DECL

/**
 * 使用指定的缓冲区作为池的内存来创建池。
 * 从池中进行的后续分配将使用此缓冲区中的内存。
 *
 * 如果参数中指定的缓冲区是位于堆栈中的缓冲区，则当执行离开包含缓冲区的封闭块时，池将无效（或隐式销毁）。请注意，应用程序必须确保在此池中分配的任何对象（如互斥）在该池失效之前已被销毁。
 *
 * @param name	    可选池名称
 * @param buf	    池要使用的缓冲区
 * @param size	    缓冲区的大小
 *
 * @return	    内存池实例
 */
PJ_DECL(pj_pool_t*) pj_pool_create_on_buf(const char *name,
					  void *buf,
					  pj_size_t size);

PJ_END_DECL

/**
 * @}
 */

#endif	/* __POOL_STACK_H__ */

