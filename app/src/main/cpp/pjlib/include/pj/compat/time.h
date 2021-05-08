/**
 * 已完成：
 *  time 操作函数相关
 *      ftime() 取得目前的时间和日期
 *      localtime()
 *          struct tm *localtime(const time_t *timer) 转换时间戳到包含年月日等信息的结构体中
 *
 */
#ifndef __PJ_COMPAT_TIME_H__
#define __PJ_COMPAT_TIME_H__

/**
 * @file time.h
 * @brief 提供 ftime() 和 localtime() 等函数
 */

#if defined(PJ_HAS_TIME_H) && PJ_HAS_TIME_H != 0
#  include <time.h>
#endif

#if defined(PJ_HAS_SYS_TIME_H) && PJ_HAS_SYS_TIME_H != 0
#  include <sys/time.h>
#endif



#endif	/* __PJ_COMPAT_TIME_H__ */

