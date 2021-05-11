/**
 * 已完成
 */
#ifndef __PJ_LIMITS_H__
#define __PJ_LIMITS_H__

/**
 * @file limits.h
 * @brief 常用最小值和最大值
 */

#include <pj/compat/limits.h>

/** 有符号32位整数的最大值 */
#define PJ_MAXINT32	0x7fffffff

/** 有符号32位整数的最小值 */
#define PJ_MININT32	0x80000000

/** 无符号16位整数的最大值  */
#define PJ_MAXUINT16	0xffff

/** 无符号字符的最大值 */
#define PJ_MAXUINT8	0xff

/** 长整型的最大值 */
#define PJ_MAXLONG	LONG_MAX

/** 长整型的最小值 */
#define PJ_MINLONG	LONG_MIN

/** 无符号长整型的最小值 */
#define PJ_MAXULONG	ULONG_MAX

#endif  /* __PJ_LIMITS_H__ */
