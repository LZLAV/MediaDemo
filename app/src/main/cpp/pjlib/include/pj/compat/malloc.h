/*
 * 已完成
 */
#ifndef __PJ_COMPAT_MALLOC_H__
#define __PJ_COMPAT_MALLOC_H__

/**
 * @file malloc.h
 * @brief 提供 malloc() 和 free() 函数
 */

#if defined(PJ_HAS_MALLOC_H) && PJ_HAS_MALLOC_H != 0
#  include <malloc.h>
#elif defined(PJ_HAS_STDLIB_H) && PJ_HAS_STDLIB_H != 0
#  include <stdlib.h>
#endif

#endif	/* __PJ_COMPAT_MALLOC_H__ */
