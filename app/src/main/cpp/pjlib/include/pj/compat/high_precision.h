/**
 * 已完成
 *  高精度数
 */
#ifndef __PJ_COMPAT_HIGH_PRECISION_H__
#define __PJ_COMPAT_HIGH_PRECISION_H__


#if defined(PJ_HAS_FLOATING_POINT) && PJ_HAS_FLOATING_POINT != 0
    /*
     * 高精度数学的第一选择是使用双精度
     */
#   include <math.h>
    typedef double pj_highprec_t;

#   define PJ_HIGHPREC_VALUE_IS_ZERO(a)     (a==0)
#   define pj_highprec_mod(a,b)             (a=fmod(a,b))

#elif defined(PJ_HAS_INT64) && PJ_HAS_INT64 != 0
    /*
     * 下一个选择是使用64位算法
     */
    typedef pj_int64_t pj_highprec_t;

#else
#   警告"高精度数学不可用"

    /*
     * 最后，回到32位算法
     */
    typedef pj_int32_t pj_highprec_t;

#endif

/**
 * @def pj_highprec_mul
 * pj_highprec_mul(a1, a2) - 高精度乘法
 * 将a1和a2相乘，结果存储在a1中
 */
#ifndef pj_highprec_mul
#   define pj_highprec_mul(a1,a2)   (a1 = a1 * a2)
#endif

/**
 * @def pj_highprec_div
 * pj_highprec_div(a1, a2) - 高精度除法
 * 将a2与a1相除，结果存储在a1中
 */
#ifndef pj_highprec_div
#   define pj_highprec_div(a1,a2)   (a1 = a1 / a2)
#endif

/**
 * @def pj_highprec_mod
 * pj_highprec_mod(a1, a2) - 高精度余数
 * a1与a2的余数，并存储到 a1
 */
#ifndef pj_highprec_mod
#   define pj_highprec_mod(a1,a2)   (a1 = a1 % a2)
#endif


/**
 * @def PJ_HIGHPREC_VALUE_IS_ZERO(a)
 * 测试指定的高精度值是否为零
 */
#ifndef PJ_HIGHPREC_VALUE_IS_ZERO
#   define PJ_HIGHPREC_VALUE_IS_ZERO(a)     (a==0)
#endif


#endif	/* __PJ_COMPAT_HIGH_PRECISION_H__ */

