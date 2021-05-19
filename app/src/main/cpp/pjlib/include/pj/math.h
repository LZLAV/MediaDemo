/**
 * 已完成：
 *  数学统计量
 */

#ifndef __PJ_MATH_H__
#define __PJ_MATH_H__

/**
 * @file math.h
 * @brief 数学和统计学
 */

#include <pj/string.h>
#include <pj/compat/high_precision.h>

PJ_BEGIN_DECL

/**
 * @defgroup pj_math 数学和统计学
 * @ingroup PJ_MISC
 * @{
 *
 * 提供常用的数学常数和运算，以及标准统计计算（最小值、最大值、平均值、标准差）。统计计算是实时完成的（统计状态在每次新样本到来时更新）
 */

/**
 * 数学常量
 */
#define PJ_PI		    3.14159265358979323846	/* pi	    */
#define PJ_1_PI		    0.318309886183790671538	/* 1/pi	    */

/**
 * 数学宏
 */
#define	PJ_ABS(x)	((x) >  0 ? (x) : -(x))
#define	PJ_MAX(x, y)	((x) > (y)? (x) : (y))
#define	PJ_MIN(x, y)	((x) < (y)? (x) : (y))

/**
 * 此结构描述统计状态
 */
typedef struct pj_math_stat
{
    int		     n;		/* 样本数	*/
    int		     max;	/* 最大值	*/
    int		     min;	/* 最小值	*/
    int		     last;	/* 最后一个值 	*/
    int		     mean;	/* 均值			*/

    /* Private members */
#if PJ_HAS_FLOATING_POINT
    float	     fmean_;	/* mean(floating point) */
#else
    int		     mean_res_;	/* mean residu		*/
#endif
    pj_highprec_t    m2_;	/* 方差	*/
} pj_math_stat;

/**
 * Calculate integer square root of an integer.
 *
 * @param i         Integer to be calculated.
 *
 * @return          Square root result.
 */
PJ_INLINE(unsigned) pj_isqrt(unsigned i)
{
    unsigned res = 1, prev;
    
    /* Rough guess, calculate half bit of input */
    prev = i >> 2;
    while (prev) {
	prev >>= 2;
	res <<= 1;
    }

    /* Babilonian method */
    do {
	prev = res;
	res = (prev + i/prev) >> 1;
    } while ((prev+res)>>1 != res);

    return res;
}

/**
 * 初始化统计
 *
 * @param stat	    统计状态
 */
PJ_INLINE(void) pj_math_stat_init(pj_math_stat *stat)
{
    pj_bzero(stat, sizeof(pj_math_stat));
}

/**
 * 当新样本出现时更新统计状态
 *
 * @param stat	    统计状态
 * @param val	    新的样本值
 */
PJ_INLINE(void) pj_math_stat_update(pj_math_stat *stat, int val)
{
#if PJ_HAS_FLOATING_POINT
    float	     delta;
#else
    int		     delta;
#endif

    stat->last = val;
    
    if (stat->n++) {
	if (stat->min > val)
	    stat->min = val;
	if (stat->max < val)
	    stat->max = val;
    } else {
	stat->min = stat->max = val;
    }

#if PJ_HAS_FLOATING_POINT
    delta = val - stat->fmean_;
    stat->fmean_ += delta/stat->n;
    
    /* Return mean value with 'rounding' */
    stat->mean = (int) (stat->fmean_ + 0.5);

    stat->m2_ += (int)(delta * (val-stat->fmean_));
#else
    delta = val - stat->mean;
    stat->mean += delta/stat->n;
    stat->mean_res_ += delta % stat->n;
    if (stat->mean_res_ >= stat->n) {
	++stat->mean;
	stat->mean_res_ -= stat->n;
    } else if (stat->mean_res_ <= -stat->n) {
	--stat->mean;
	stat->mean_res_ += stat->n;
    }

    stat->m2_ += delta * (val-stat->mean);
#endif
}

/**
 * 获取指定统计状态的标准差
 *
 * @param stat	    统计状态
 *
 * @return	    标准差
 */
PJ_INLINE(unsigned) pj_math_stat_get_stddev(const pj_math_stat *stat)
{
    if (stat->n == 0) return 0;
    return (pj_isqrt((unsigned)(stat->m2_/stat->n)));
}

/**
 * 设置统计状态的标准差。当统计状态以“只读”模式作为统计数据的存储时，这很有用
 *
 * @param stat	    统计状态
 *
 * @param dev	    标准差
 */
PJ_INLINE(void) pj_math_stat_set_stddev(pj_math_stat *stat, unsigned dev)
{
    if (stat->n == 0) 
	stat->n = 1;
    stat->m2_ = dev*dev*stat->n;
}

/** @} */

PJ_END_DECL

#endif /* __PJ_MATH_H__ */
