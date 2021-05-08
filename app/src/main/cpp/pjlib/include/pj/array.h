/**
 * 已完成
 * 	数组的插入、删除和搜索
 */
#ifndef __PJ_ARRAY_H__
#define __PJ_ARRAY_H__

/**
 * @file array.h
 * @brief PJLIB 数组帮助
 */
#include <pj/types.h>

PJ_BEGIN_DECL

/**
 * @defgroup PJ_ARRAY 数组帮助
 * @ingroup PJ_DS
 * @{
 *
 * 此模块提供帮助程序来操作任意大小的元素数组。它提供最常用的数组操作，如插入、擦除和搜索
 */

/**
 * 在给定位置向数组中插入值，并在该位置后重新排列其余节点
 *
 * @param array	    数组
 * @param elem_size 单个元素的大小
 * @param count	    数组中当前的元素数
 * @param pos	    新元素放置的位置
 * @param value	    要复制到新元素的值
 */
PJ_DECL(void) pj_array_insert( void *array,
			       unsigned elem_size,
			       unsigned count,
			       unsigned pos,
			       const void *value);

/**
 * 在给定位置从数组中删除一个值，并在删除元素后重新排列其余元素
 *
 * @param array	    数组
 * @param elem_size 单个元素的大小
 * @param count	    数组中当前的元素数
 * @param pos	    要删除的索引/位置
 */
PJ_DECL(void) pj_array_erase( void *array,
			      unsigned elem_size,
			      unsigned count,
			      unsigned pos);

/**
 * 根据匹配函数搜索数组中的第一个值
 *
 * @param array	    数组
 * @param elem_size 单个元素的大小
 * @param count	    元素的数量
 * @param matching  匹配函数，如果指定的元素匹配，则必须返回PJ_SUCCESS
 * @param result    指向找到的值的指针
 *
 * @return	    值找到返回 PJ_SUCCESS，否则返回错误码
 */
PJ_DECL(pj_status_t) pj_array_find(   const void *array, 
				      unsigned elem_size, 
				      unsigned count, 
				      pj_status_t (*matching)(const void *value),
				      void **result);

/**
 * @}
 */

PJ_END_DECL


#endif	/* __PJ_ARRAY_H__ */

