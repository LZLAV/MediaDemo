/* $Id: assert.h 3553 2011-05-05 06:14:19Z nanang $ */
/* 
 * Copyright (C) 2008-2011 Teluu Inc. (http://www.teluu.com)
 * Copyright (C) 2003-2008 Benny Prijono <benny@prijono.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */
#ifndef __PJ_ASSERT_H__
#define __PJ_ASSERT_H__

/**
 * @file assert.h
 * @brief 断言宏pj_assert()
 */

#include <pj/config.h>
#include <pj/compat/assert.h>

/**
 * @defgroup pj_assert 断言宏
 * @ingroup PJ_MISC
 * @{
 *
 * 断言和其他辅助宏用于健全性检查
 */

/**
 * @hideinitializer
 * 在调试生成期间检查表达式是否为 true。如果表达式在运行时计算为 false，那么程序将在出现问题的语句处停止。
 * 对于release build，此宏不会执行任何操作
 *
 * @param expr	    要计算的表达式
 */
#ifndef pj_assert
#   define pj_assert(expr)   assert(expr)
#endif


/**
 * @hideinitializer
 * 如果声明了 PJ_ENABLE_EXTRA_CHECK 且值非零，则 PJ_ASSERT_RETURN 宏将在运行时对
 * @a expr中的表达式求值。如果表达式产生false，将触发断言，当前函数将返回指定的返回值。
 * 如果 PJ_ENABLE_EXTRA_CHECK 未声明或为零，则不会执行运行时检查。宏的计算结果只是 pj_assert(expr)
 */
#if defined(PJ_ENABLE_EXTRA_CHECK) && PJ_ENABLE_EXTRA_CHECK != 0
#   define PJ_ASSERT_RETURN(expr,retval)    \
	    do { \
		if (!(expr)) { pj_assert(expr); return retval; } \
	    } while (0)
#else
#   define PJ_ASSERT_RETURN(expr,retval)    pj_assert(expr)
#endif

/**
 * @hideinitializer
 * 如果声明了 PJ_ENABLE_EXTRA_CHECK 且非零，则 PJ_ASSERT_ON_FAIL 宏将在运行时对
 * @a expr中的表达式求值。如果表达式产生 false，将触发断言并执行@a exec_on_fail
 * 如果 PJ_ENABLE_EXTRA_CHECK 未声明或为零，则不会执行运行时检查。宏的计算结果只是pj_assert(expr)
 */
#if defined(PJ_ENABLE_EXTRA_CHECK) && PJ_ENABLE_EXTRA_CHECK != 0
#   define PJ_ASSERT_ON_FAIL(expr,exec_on_fail)    \
	    do { \
		pj_assert(expr); \
		if (!(expr)) exec_on_fail; \
	    } while (0)
#else
#   define PJ_ASSERT_ON_FAIL(expr,exec_on_fail)    pj_assert(expr)
#endif

/** @} */

#endif	/* __PJ_ASSERT_H__ */

