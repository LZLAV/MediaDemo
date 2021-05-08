/**
 * 已完成
 *  整数限制
 *      无符号、有符号 最大值和最小值
 *
 */
#ifndef __PJ_COMPAT_LIMITS_H__
#define __PJ_COMPAT_LIMITS_H__

/**
 * @file limits.h
 * @brief 提供通常在limits.h中找到的整数限制
 */

#if defined(PJ_HAS_LIMITS_H) && PJ_HAS_LIMITS_H != 0
#  include <limits.h>
#else

#  ifdef _MSC_VER
#  pragma message("limits.h is not found or not supported. LONG_MIN and "\
		 "LONG_MAX will be defined by the library in "\
		 "pj/compats/limits.h and overridable in config_site.h")
#  else
#  warning "limits.h is not found or not supported. LONG_MIN and LONG_MAX " \
           "will be defined by the library in pj/compats/limits.h and "\
           "overridable in config_site.h"
#  endif

/* "signed long int"可以容纳的最小值和最大值  */
#  ifndef LONG_MAX
#    if __WORDSIZE == 64
#      define LONG_MAX     9223372036854775807L
#    else
#      define LONG_MAX     2147483647L
#    endif
#  endif

#  ifndef LONG_MIN
#    define LONG_MIN      (-LONG_MAX - 1L)
#  endif

/* "unsigned long int"可以容纳的最大值(最小值为0）  */
#  ifndef ULONG_MAX
#    if __WORDSIZE == 64
#      define ULONG_MAX    18446744073709551615UL
#    else    
#      define ULONG_MAX    4294967295UL
#    endif
#  endif
#endif

#endif  /* __PJ_COMPAT_LIMITS_H__ */
