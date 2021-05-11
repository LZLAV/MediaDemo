/* $Id: list.h 4624 2013-10-21 06:37:30Z ming $ */
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
#ifndef __PJ_LIST_H__
#define __PJ_LIST_H__

/**
 * @file list.h
 * @brief 链表数据结构
 */

#include <pj/types.h>

PJ_BEGIN_DECL

/*
 * @defgroup PJ_DS 数据结构
 */

/**
 * @defgroup PJ_LIST 链表
 * @ingroup PJ_DS
 * @{
 *
 * PJLIB中的 List 实现为双链表，它不需要动态内存分配（就像所有PJLIB数据结构一样）。此处的列表应该更像是一个低级别的C列表，
 * 而不是高级C++列表（通常更容易使用，但需要动态内存分配），因此所有C列表的注意事项也适用于这里（比如，不能放置一个节点到多个列表中）
 *
 * \section pj_list_example_sec 例子
 *
 * 有关如何操作链表的示例，请参见下文：
 *  - @ref page_pjlib_samples_list_c
 *  - @ref page_pjlib_list_test
 */


/**
 * 在结构声明的开头使用此宏可以声明该结构可用于链表操作。这个宏只是附加结构声明 prev和 next 成员
 * @hideinitializer
 */
#define PJ_DECL_LIST_MEMBER(type)                       \
                                   /** List @a prev. */ \
                                   type *prev;          \
                                   /** List @a next. */ \
                                   type *next 


/**
 * 此结构描述通用列表节点和列表。此列表的所有者必须将 'value' 成员初始化为适当的值（通常是所有者本身）
 */
struct pj_list
{
    PJ_DECL_LIST_MEMBER(void);
} PJ_ATTR_MAY_ALIAS; /* may_alias avoids warning with gcc-4.4 -Wall -O2 */


/**
 * 初始化列表
 * 最初，列表将没有成员，函数 pj_list_empty() 将始终为新初始化的列表返回非零（表示TRUE）
 *
 * @param node 链表头
 */
PJ_INLINE(void) pj_list_init(pj_list_type * node)
{
    ((pj_list*)node)->next = ((pj_list*)node)->prev = node;
}


/**
 * 检查链表是否为空表
 *
 * @param node	链表头
 *
 * @return  空返回非零值，否则为零值
 *
 */
PJ_INLINE(int) pj_list_empty(const pj_list_type * node)
{
    return ((pj_list*)node)->next == node;
}


/**
 * 将节点插入到指定元素位置之前的列表中
 *
 * @param pos	将在其前面插入节点的元素
 * @param node	待插入的元素
 *
 * @return void.
 */
PJ_IDECL(void)	pj_list_insert_before(pj_list_type *pos, pj_list_type *node);


/**
 * 将节点插入列表的后面。这只是 pj_list_insert_before() 的别名
 *
 * @param list	链表
 * @param node	待插入的元素
 */
PJ_INLINE(void) pj_list_push_back(pj_list_type *list, pj_list_type *node)
{
    pj_list_insert_before(list, node);
}


/**
 * 将节点列表中的节点插入到目标链表
 *
 * @param lst	    目标链表
 * @param nodes	    节点列表
 */
PJ_IDECL(void) pj_list_insert_nodes_before(pj_list_type *lst,
					   pj_list_type *nodes);

/**
 * 在指定的元素位置之后向列表中插入节点
 *
 * @param pos	    列表中位于插入元素之前的元素
 * @param node	    要插入位置元素之后的元素
 *
 * @return void.
 */
PJ_IDECL(void) pj_list_insert_after(pj_list_type *pos, pj_list_type *node);


/**
 * 将节点插入列表的前面。这只是 pj_list_insert_after() 的别名
 *
 * @param list	链表
 * @param node	要插入的元素
 */
PJ_INLINE(void) pj_list_push_front(pj_list_type *list, pj_list_type *node)
{
    pj_list_insert_after(list, node);
}


/**
 * 将节点列表中的所有节点插入目标列表
 *
 * @param lst	    目标链表
 * @param nodes	    节点列表
 */
PJ_IDECL(void) pj_list_insert_nodes_after(pj_list_type *lst,
					  pj_list_type *nodes);


/**
 * 从源列表中删除元素，然后将它们插入到目标列表中。源列表的元素将占据目标列表的前元素。
 * 请注意，list2 本身所指向的节点不被视为节点，而是作为列表描述符，因此它不会被插入list1。
 * 要插入的元素从 list2->next 开始。如果要在操作中包括 list2，请使用 pj_list_insert_nodes_before
 *
 * @param list1	目标链表
 * @param list2	源链表
 *
 * @return void.
 */
PJ_IDECL(void) pj_list_merge_first(pj_list_type *list1, pj_list_type *list2);


/**
 * 从第二个 list 参数中删除元素，并将它们插入到第一个参数中的列表中。第二个列表中的元素将附加到第一个列表中。
 * 请注意，list2 本身所指向的节点不被视为节点，而是作为列表描述符，因此它不会被插入 list1。要插入的元素从
 * list2->next开始。如果要在操作中包括 list2，请使用 pj_list_insert_nodes_before
 *
 * @param list1	    列表中位于插入元素之前的元素
 * @param list2	    要插入的列表中的元素
 *
 * @return void.
 */
PJ_IDECL(void) pj_list_merge_last( pj_list_type *list1, pj_list_type *list2);


/**
 * 从节点当前所属的列表中删除该节点
 *
 * @param node	    要删除的元素
 */
PJ_IDECL(void) pj_list_erase(pj_list_type *node);


/**
 * 在列表中查找节点
 *
 * @param list	    链表头
 * @param node	    要搜索的节点元素
 *
 * @return  如果在列表中找到节点，则返回节点本身；如果在列表中找不到节点，则返回NULL
 */
PJ_IDECL(pj_list_type*) pj_list_find_node(pj_list_type *list, 
					  pj_list_type *node);


/**
 * 使用指定的比较函数在列表中搜索指定的值。此函数在列表中的节点上迭代，
 * 从第一个节点开始，并调用用户提供的比较函数，直到比较函数返回零
 *
 * @param list	    链表头
 * @param value	    要在比较函数中传递的用户定义值
 * @param comp	    比较函数，该函数应返回 0 以指示找到了搜索的值
 *
 * @return  匹配的第一个节点，如果找不到则为NULL
 */
PJ_IDECL(pj_list_type*) pj_list_search(pj_list_type *list, void *value,
				       int (*comp)(void *value, 
						   const pj_list_type *node)
				       );


/**
 * 遍历列表以获取列表中的元素数
 *
 * @param list	    链表头
 *
 * @return	    元素数
 */
PJ_IDECL(pj_size_t) pj_list_size(const pj_list_type *list);


/**
 * @}
 */

#if PJ_FUNCTIONS_ARE_INLINED
#  include "list_i.h"
#endif

PJ_END_DECL

#endif	/* __PJ_LIST_H__ */

