/**
 * 已完成：
 *  stddef .h 头文件定义了各种变量类型和宏
 *  ptrdiff_t
 *      这是有符号整数类型，它是两个指针相减的结果
 *  size_t
 *      这是无符号整数类型，它是 sizeof 关键字的结果
 *  wchar_t
 *      这是一个宽字符常量大小的整数类型
 *  NULL
 *      这个宏是一个空指针常量的值
 */
#ifndef __PJ_COMPAT_SIZE_T_H__
#define __PJ_COMPAT_SIZE_T_H__

/**
 * @file size_t.h
 * @brief 提供 size_t 类型
 */
#if PJ_HAS_STDDEF_H
# include <stddef.h>
#endif

#endif	/* __PJ_COMPAT_SIZE_T_H__ */

