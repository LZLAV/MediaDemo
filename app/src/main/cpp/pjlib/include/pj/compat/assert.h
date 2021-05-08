/**
 * 已完成：
 *  assert() 宏的原型定义在 assert.h 中，其作用是如果它的条件返回错误，则终止程序执行
 *
 *  void assert( int expression );
 *      assert 的作用是现计算表达式 expression ，如果其值为假（即为0），那么它先向 stderr 打印一条出错信息,然后通过调用 abort 来终止程序运行。
 *  使用 assert 的缺点是，频繁的调用会极大的影响程序的性能，增加额外的开销。
 */
#ifndef __PJ_COMPAT_ASSERT_H__
#define __PJ_COMPAT_ASSERT_H__

/**
 * @file assert.h
 * @brief 提供 assert() 宏
 */

#if defined(PJ_HAS_ASSERT_H) && PJ_HAS_ASSERT_H != 0
#  include <assert.h>

#else
#  warning "assert() is not implemented"
#  define assert(expr)
#endif

#endif	/* __PJ_COMPAT_ASSERT_H__ */

