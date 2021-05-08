/**
 * 已完成：
 *  伪随机数
 *      rand()函数是使用线性同余法做的，它并不是真的随机数，因为其周期特别长，所以在一定范围内可以看成随机的。
 *      rand()函数不需要参数，它将会返回 0 到 RAND_MAX 之间的任意的整数。
 *
 *      srand()为初始化随机数发生器，用于设置 rand()产生随机数时的种子。传入的参数 seed 为unsigned int类型，
 *      通常我们会使用 time(0) 或 time(NULL) 的返回值作为seed。
 *
 *      默认为 srand(1)
 *
 *      srand(time(0))
 *          time(0) 返回的是从1970 UTC Jan 1 00:00到当前时刻的秒数，为unsigned int类型
 *          两次调用srand()函数设置随机数种子之间的时间间隔不超过1s，这会导致我们重置随机数种子，从而等价于使用了一个固定的随机数种子。
 *
 *      固定种子，多次执行所产生的随机数会一致
 */
#ifndef __PJ_COMPAT_RAND_H__
#define __PJ_COMPAT_RAND_H__

/**
 * @file rand.h
 * @brief 提供 platform_rand() 和 platform_srand() 函数
 */

#if defined(PJ_HAS_STDLIB_H) && PJ_HAS_STDLIB_H != 0
   /*
    * 使用 stdlib 的 rand() 和 srand().
    */
#  include <stdlib.h>
#  define platform_srand    srand
#  if defined(RAND_MAX) && RAND_MAX <= 0xFFFF
       /*
        * 当rand（）只有16位强函数时，通过两次调用它来加倍强函数！
	*/
       PJ_INLINE(int) platform_rand(void)
       {
	   return ((rand() & 0xFFFF) << 16) | (rand() & 0xFFFF);
       }
#  else
#      define platform_rand rand
#  endif

#else
#  warning "platform_rand() is not implemented"
#  define platform_rand()	1
#  define platform_srand(seed)

#endif


#endif	/* __PJ_COMPAT_RAND_H__ */

