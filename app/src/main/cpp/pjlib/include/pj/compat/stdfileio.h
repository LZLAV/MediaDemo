/**
 * 已完成
 */
#ifndef __PJ_COMPAT_STDFILEIO_H__
#define __PJ_COMPAT_STDFILEIO_H__

/**
 * @file stdfileio.h
 * @brief 兼容 ANSI 文件I/O，如fput、fflush等
 */

#if defined(PJ_HAS_STDIO_H) && PJ_HAS_STDIO_H != 0
#  include <stdio.h>
#endif

#endif	/* __PJ_COMPAT_STDFILEIO_H__ */
