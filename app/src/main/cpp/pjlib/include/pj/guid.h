/**
 * 已完成
 *      GUID  全局唯一标识符
 *          字符串的唯一标识符
 */
#ifndef __PJ_GUID_H__
#define __PJ_GUID_H__


/**
 * @file guid.h
 * @brief GUID 全局唯一标识符
 */
#include <pj/types.h>


PJ_BEGIN_DECL


/**
 * @defgroup PJ_DS 数据结构
 */
/**
 * @defgroup PJ_GUID 全局唯一标识符
 * @ingroup PJ_DS
 * @{
 *
 * 此模块提供 API 来创建全局唯一的字符串。
 * 如果应用程序不需要这个强大的需求，它可以使用 pj_create_random_string() 来代替
 */


/**
 * PJ_GUID_STRING_LENGTH 指定GUID字符串的长度。该值取决于内部用于生成 GUID字符串的算法。如果使用实 GUID生成器，
 * 则长度将在32到36字节之间。应用程序不应假定GUID生成器将使用哪个算法。不管GUID的实际长度是多少，它都不会超过
 * PJ_GUID_MAX_LENGTH 字符
 *
 * @see pj_GUID_STRING_LENGTH()
 * @see PJ_GUID_MAX_LENGTH
 */
PJ_DECL_DATA(const unsigned) PJ_GUID_STRING_LENGTH;

/**
 * 得到 PJ_GUID_STRING_LENGTH 常量
 */
PJ_DECL(unsigned) pj_GUID_STRING_LENGTH(void);

/**
 * PJ_GUID_MAX_LENGTH 指定 GUID 字符串的最大长度，无论使用哪种算法
 */
#define PJ_GUID_MAX_LENGTH  36

/**
 * 创建一个全局唯一的字符串，长度为 PJ_GUID_STRING_LENGTH 个字符。调用者负责预先分配字符串中使用的存储
 *
 * @param str       存储结果的字符串
 *
 * @return          字符串
 */
PJ_DECL(pj_str_t*) pj_generate_unique_string(pj_str_t *str);

/**
 * 创建一个全局唯一的小写字符串，长度为 PJ_GUID_STRING_LENGTH 个字符。调用者负责预先分配字符串中使用的存储
 *
 * @param str       存储结果的字符串
 *
 * @return          字符串
 */
PJ_DECL(pj_str_t*) pj_generate_unique_string_lower(pj_str_t *str);

/**
 * 生成一个唯一的字符串
 *
 * @param pool	    分配内存的内存池
 * @param str	    字符串
 */
PJ_DECL(void) pj_create_unique_string(pj_pool_t *pool, pj_str_t *str);

/**
 * 生成一个全小写的唯一字符串
 *
 * @param pool	    内存池
 * @param str	    字符串
 */
PJ_DECL(void) pj_create_unique_string_lower(pj_pool_t *pool, pj_str_t *str);


/**
 * @}
 */

PJ_END_DECL

#endif/* __PJ_GUID_H__ */

