/* $Id: hash.h 4208 2012-07-18 07:52:33Z ming $ */
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
#ifndef __PJ_HASH_H__
#define __PJ_HASH_H__

/**
 * @file hash.h
 * @brief 哈希表
 */

#include <pj/types.h>

PJ_BEGIN_DECL

/**
 * @defgroup PJ_HASH 哈希表
 * @ingroup PJ_DS
 * @{
 * 哈希表是一种字典，其中键通过哈希函数映射到数组位置。将多个项目的关键点映射到同一位置称为碰撞。
 * 在这个库中，我们将链接列表中具有相同键的节点
 */

/**
 * 如果该常量用作 keylen，则该键将被解释为以 NULL结尾的字符串
 */
#define PJ_HASH_KEY_STRING	((unsigned)-1)

/**
 * 这表示每个哈希项的大小
 */
#define PJ_HASH_ENTRY_BUF_SIZE	(3*sizeof(void*) + 2*sizeof(pj_uint32_t))

/**
 * 条目缓冲区的类型声明，由 pj_hash_set_np() 使用
 */
typedef void *pj_hash_entry_buf[(PJ_HASH_ENTRY_BUF_SIZE+sizeof(void*)-1)/(sizeof(void*))];

/**
 * 这是哈希表用来计算指定键的哈希值的函数
 *
 * @param hval	    初始哈希值，或零
 * @param key	    计算的key
 * @param keylen    密钥的长度，或 PJ_HASH_KEY_STRING 将密钥视为以 null结尾的字符串
 *
 * @return          哈希值
 */
PJ_DECL(pj_uint32_t) pj_hash_calc(pj_uint32_t hval, 
				  const void *key, unsigned keylen);


/**
 * 将密钥转换为小写并计算哈希值。结果字符串存储在结果中
 *
 * @param hval      初始散列值，通常为零
 * @param result    可选。缓冲区来存储结果，结果必须足以容纳字符串
 * @param key       要转换和计算的输入key
 *
 * @return          哈希值
 */
PJ_DECL(pj_uint32_t) pj_hash_calc_tolower(pj_uint32_t hval,
                                          char *result,
                                          const pj_str_t *key);

/**
 * 创建具有指定 'bucket' 大小的哈希表
 *
 * @param pool	从中分配哈希表的池
 * @param size	桶大小，将四舍五入到最接近的 2^n-1
 *
 * @return 哈希表
 */
PJ_DECL(pj_hash_table_t*) pj_hash_create(pj_pool_t *pool, unsigned size);


/**
 * 获取与指定键关联的值
 *
 * @param ht	    哈希表
 * @param key	    要查找的key
 * @param keylen    key 的长度，或 PJ_HASH_KEY_STRING来使用密钥的字符串长度
 * @param hval	    如果此参数不为NULL且值不为零，则该值将用作计算的哈希值。如果参数不为NULL且值为零，
 * 					则返回时将用计算的哈希值填充
 *
 * @return 与键关联的值，如果找不到键，则为NULL
 */
PJ_DECL(void *) pj_hash_get( pj_hash_table_t *ht,
			     const void *key, unsigned keylen,
			     pj_uint32_t *hval );


/**
 * pj_hash_get() 的变体，在计算哈希值时将键转换为小写
 *
 * @see pj_hash_get()
 */
PJ_DECL(void *) pj_hash_get_lower( pj_hash_table_t *ht,
			           const void *key, unsigned keylen,
			           pj_uint32_t *hval );


/**
 * 将值与指定键关联/取消关联。如果值不为NULL且条目已存在，则将覆盖该条目的值。
 * 如果值不为NULL且条目不存在，则将使用指定的池创建一个新条目。否则，如果值为空，则条目将被删除（如果存在）
 *
 * @param pool	    如果必须创建新条目，则分配新条目的池
 * @param ht	    哈希表
 * @param key	    key。如果未指定pool，则键必须指向在条目期间保持有效的缓冲区
 * @param keylen    key的长度，或 PJ_HASH_KEY_STRING 来使用密钥的字符串长度
 * @param hval		如果该值不为零，则哈希表将使用该值搜索条目的索引，否则将计算 key。调用 pj_hash_get() 时可以获取此值
 * @param value	    要关联的值，或 NULL以删除具有指定键的条目
 */
PJ_DECL(void) pj_hash_set( pj_pool_t *pool, pj_hash_table_t *ht,
			   const void *key, unsigned keylen, pj_uint32_t hval,
			   void *value );


/**
 * pj_hash_set() 的变体，在计算哈希值时将键转换为小写
 *
 * @see pj_hash_set()
 */
PJ_DECL(void) pj_hash_set_lower( pj_pool_t *pool, pj_hash_table_t *ht,
			         const void *key, unsigned keylen,
                                 pj_uint32_t hval, void *value );


/**
 * Associate/disassociate a value with the specified key. This function works
 * like #pj_hash_set(), except that it doesn't use pool (hence the np -- no 
 * pool suffix). If new entry needs to be allocated, it will use the entry_buf.
 *
 * @param ht	    the hash table.
 * @param key	    the key.
 * @param keylen    the length of the key, or PJ_HASH_KEY_STRING to use the 
 *		    string length of the key.
 * @param hval	    if the value is not zero, then the hash table will use
 *		    this value to search the entry's index, otherwise it will
 *		    compute the key. This value can be obtained when calling
 *		    #pj_hash_get().
 * @param entry_buf Buffer which will be used for the new entry, when one needs
 *		    to be created.
 * @param value	    value to be associated, or NULL to delete the entry with
 *		    the specified key.
 */
PJ_DECL(void) pj_hash_set_np(pj_hash_table_t *ht,
			     const void *key, unsigned keylen, 
			     pj_uint32_t hval, pj_hash_entry_buf entry_buf, 
			     void *value);

/**
 * Variant of #pj_hash_set_np() with the key being converted to lowercase
 * when calculating the hash value.
 *
 * @see pj_hash_set_np()
 */
PJ_DECL(void) pj_hash_set_np_lower(pj_hash_table_t *ht,
			           const void *key, unsigned keylen,
			           pj_uint32_t hval,
                                   pj_hash_entry_buf entry_buf,
			           void *value);

/**
 * Get the total number of entries in the hash table.
 *
 * @param ht	the hash table.
 *
 * @return the number of entries in the hash table.
 */
PJ_DECL(unsigned) pj_hash_count( pj_hash_table_t *ht );


/**
 * Get the iterator to the first element in the hash table. 
 *
 * @param ht	the hash table.
 * @param it	the iterator for iterating hash elements.
 *
 * @return the iterator to the hash element, or NULL if no element presents.
 */
PJ_DECL(pj_hash_iterator_t*) pj_hash_first( pj_hash_table_t *ht,
					    pj_hash_iterator_t *it );


/**
 * Get the next element from the iterator.
 *
 * @param ht	the hash table.
 * @param it	the hash iterator.
 *
 * @return the next iterator, or NULL if there's no more element.
 */
PJ_DECL(pj_hash_iterator_t*) pj_hash_next( pj_hash_table_t *ht, 
					   pj_hash_iterator_t *it );

/**
 * Get the value associated with a hash iterator.
 *
 * @param ht	the hash table.
 * @param it	the hash iterator.
 *
 * @return the value associated with the current element in iterator.
 */
PJ_DECL(void*) pj_hash_this( pj_hash_table_t *ht,
			     pj_hash_iterator_t *it );


/**
 * @}
 */

PJ_END_DECL

#endif


