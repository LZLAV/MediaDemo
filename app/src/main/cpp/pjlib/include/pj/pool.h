/* $Id: pool.h 5533 2017-01-19 06:10:15Z nanang $ */
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

#include <pj/list.h>

/*
 * 是否使用pool的备用API
 * 备用API用于实现内存池调试
 */
#if PJ_HAS_POOL_ALT_API
#  include <pj/pool_alt.h>
#endif


#ifndef __PJ_POOL_H__
#define __PJ_POOL_H__

/**
 * @file pool.h
 * @brief 内存池
 */

PJ_BEGIN_DECL

/**
 * @defgroup PJ_POOL_GROUP 快速内存池
 * @brief
 * 内存池允许动态内存分配与 malloc 或 new 相媲美。这些实现对于高性能的应用程序或实时系统来说是不可取的，
 * 因为存在性能瓶颈，而且还存在碎片问题
 *
 * \section PJ_POOL_INTRO_SEC PJLIB's 内存池
 * \subsection PJ_POOL_ADVANTAGE_SUBSEC 优势
 *
 *
 * 与传统的malloc/new操作符和其他内存池实现相比，PJLIB的池具有许多优势，因为：
 *  -与其他内存池实现不同，它允许分配不同大小的内存块，
 *  -非常非常快。
 *      内存块分配不仅是一个O(1)操作，而且非常简单（只有几个指针算术运算），而且不需要锁定，任何互斥锁，
 *  -高效的内存
 *      Pool不跟踪应用程序分配的单个内存块，因此每个内存分配不需要额外的开销（除了可能额外的几个字节，最多为 PJ_POOL_ALIGNMENT-1，用于对齐内存）。
 *      但请参见下面的@ref PJ_POOL_CAVEATS_SUBSEC
 *  -它可以防止内存泄漏。
 *      内存池本身就具有垃圾收集功能。实际上，不需要释放内存池中分配的块。
 *      一旦池本身被销毁，以前从池中分配的所有块都将被释放。这将防止数十年来困扰程序员的内存泄漏，并且与传统的 malloc/new操作符相比，它提供了额外的性能优势。
 *  更重要的是，PJLIB的内存池为应用程序提供了一些额外的可用性和灵活性：
 *      -内存泄漏很容易追踪，因为内存池被分配了名称，应用程序可以检查系统中当前活动的池。
 *      -根据设计，来自池的内存分配不是线程安全的。我们假设池将由更高级别的对象拥有，线程安全应该由该对象处理。这样可以实现非常快速的池操作，并防止不必要的锁定操作，
 *      -默认情况下，内存池 API 的行为更像 C++ 新操作符，因为当内存块分配失败时，它将抛出 PJ_NO_MEMORY_EXCEPTION 异常（参见@ REF PJ_EXCEPT）。
 *          这使得故障处理可以在更高级的函数上完成（而不是每次都检查 pj_pool_alloc() 的结果）。如果应用程序不喜欢这样，可以通过向池工厂提供不同的策略，在全局基础上更改默认行为。
 *      -可以使用任何内存分配后端分配器/释放定位器。默认情况下，策略使用malloc（）和free（）来管理池的块，但应用程序可能使用不同的策略，例如从全局静态内存位置分配内存块
 *
 *
 * \subsection PJ_POOL_PERFORMANCE_SUBSEC 性能
 * 
 * PJLIB的内存设计和精心实现的结果是一种内存分配策略，与标准 malloc()/free()（在 P4/3.0GHz Linux机器上每秒分配超过1.5亿次）相比，它可以将内存分配和释放速度提高30倍。
 * （注意：当然，您的里程数可能会有所不同。您可以通过运行 pjlib-test 应用程序，了解PJLIB池在目标系统中比 malloc()/free() 提高性能的程度。

创建池时，PJLIB要求应用程序指定初始池大小，并且一旦创建池，PJLIB就按该大小从系统分配内存。应用程序设计者必须仔细选择初始池大小，因为选择太大的值会导致系统内存的浪费。
但游泳池会长出来。应用程序设计器可以通过指定创建池时的大小增量来指定池的大小增长方式。
但是，池不能</b>收缩！由于没有释放内存块的功能，因此池无法将未使用的内存释放回系统。
应用程序设计者必须注意，来自具有无限生命周期的池中的恒定内存分配可能会导致应用程序的内存使用量随时间的推移而增长。

 * \subsection PJ_POOL_CAVEATS_SUBSEC 附加说明
 *
 * 还有一些附加说明！
 *  创建池时，PJLIB要求应用程序指定初始池大小，并且一旦创建池，PJLIB就按该大小从系统分配内存。应用程序设计者必须仔细选择初始池大小，因为选择太大的值会导致系统内存的浪费
 *  但内存池是可以增长的。开发人员可以通过指定创建池时的大小增量来指定池的大小增长方式
 *  但是，内存池不能收缩！由于没有释放内存块的功能，因此池无法将未使用的内存释放回系统
 *  开发人员必须注意，来自具有无限生命周期的池中的恒定内存分配可能会导致应用程序的内存使用量随时间的推移而增长
 *
 * \section PJ_POOL_USING_SEC 内存池的使用
 *
 * 本节介绍如何使用PJLIB的内存池框架。
 * 我们希望读者能够看到，PJLIB的内存池API非常简单。
 *
 * PJ_POOL_USING_F 创建池工厂
 *  首先，应用程序需要初始化池工厂（这通常只需要在一个应用程序中完成一次）。PJLIB 提供了一个名为 caching pool 的池工厂实现（请参见@ref PJ_caching_pool），
 *  它通过调用 pj_caching_pool_init() 进行初始化。
 * PJ_POOL_USING_P 创建池
 *  然后应用程序用 pj_pool_create() 创建池对象本身，并指定创建池的池工厂、池名称、初始大小和增量/扩展大小。
 * PJ_POOL_USING_M 根据需要分配内存
 *  然后，每当应用程序需要分配动态内存时，它都会调用 pj_pool_alloc()、pj_pool_calloc()或 pj_pool_zalloc()从池中分配内存块。
 * PJ_POOL_USING_DP 销毁池
 *  应用程序处理完池后，应该调用#pj_pool_release（），将池对象释放回工厂。根据工厂的类型，这可能会将内存释放回操作系统。
 * PJ_POOL_USING_Dc 销毁内存池工厂
 *  最后，在应用程序退出之前，应该取消池工厂的初始化，以确保工厂分配的所有内存块都释放回操作系统。在此之后，当然不能再请求内存池分配了。
 * PJ_POOL_USING_EX 示例
 *  下面是一个利用PJLIB内存池的完整程序示例
 *
 * \code

   #include <pjlib.h>

   #define THIS_FILE    "pool_sample.c"

   static void my_perror(const char *title, pj_status_t status)
   {
        char errmsg[PJ_ERR_MSG_SIZE];

	pj_strerror(status, errmsg, sizeof(errmsg));
	PJ_LOG(1,(THIS_FILE, "%s: %s [status=%d]", title, errmsg, status));
   }

   static void pool_demo_1(pj_pool_factory *pfactory)
   {
	unsigned i;
	pj_pool_t *pool;

	// 必须先创建池才能分配任何内容
	pool = pj_pool_create(pfactory,	 // the factory
			      "pool1",	 // pool's name
			      4000,	 // 初始化大小
			      4000,	 // 增量大小
			      NULL);	 // 使用默认的回调
	if (pool == NULL) {
	    my_perror("Error creating pool", PJ_ENOMEM);
	    return;
	}

	// Demo: 分配一些内存块
	for (i=0; i<1000; ++i) {
	    void *p;

	    p = pj_pool_alloc(pool, (pj_rand()+1) % 512);

	    // p 的一些操作
	    ...

	    // 不需要释放 p
	}

	// 必须释放内存池去释放所有内存
	pj_pool_release(pool);
   }

   int main()
   {
	pj_caching_pool cp;
	pj_status_t status;

        // 必须提前初始化 PJLIB
	status = pj_init();
	if (status != PJ_SUCCESS) {
	    my_perror("Error initializing PJLIB", status);
	    return 1;
	}

	//  使用默认池策略创建池工厂（在本例中为缓存池）。
	pj_caching_pool_init(&cp, NULL, 1024*1024 );

	// Do a demo
	pool_demo_1(&cp.factory);

	// 应用退出，销毁缓存池
	pj_caching_pool_destroy(&cp);

	return 0;
   }

   \endcode
 *
 * 关于池工厂、池对象和缓存池的更多信息可以在下面的模块链接中找到
 */


/**
 * @defgroup PJ_POOL 内存池对象
 * @ingroup PJ_POOL_GROUP
 * @brief
 * 内存池是由池工厂创建的不透明对象。
 * 应用程序通过调用 pj_pool_alloc()、pj_pool_calloc() 或 pj_pool_zalloc()，使用此对象请求内存块。应用程序使用完池后，必须调用 pj_pool_release()，
 * 释放先前分配的所有块，并将池释放回工厂。
 *
 * 内存池是用初始内存量初始化的，称为块。池可以配置为在内存耗尽时动态分配更多内存块。
 * 池不按用户跟踪单个内存分配，用户也不必释放这些独立的分配。这使得内存分配简单且非常快速。当池本身被销毁时，从池中分配的所有内存都将被销毁
 *
 * \section PJ_POOL_THREADING_SEC 有关线程策略的详细信息
 * -根据设计，来自池的内存分配不是线程安全的。我们假设一个池将由一个对象拥有，线程安全应该由该对象处理。因此，这些函数不是线程安全的：
 *  - pj_pool_alloc
 *  - pj_pool_calloc
 *  -以及其他池统计函数。
 *  -池工厂中的线程由创建工厂时为工厂设置的策略决定。
 *
 * \section PJ_POOL_EXAMPLES_SEC 示例
 *
 * 有关如何使用池的一些示例代码，请参见：
 *  - @ref page_pjlib_pool_test
 *
 * @{
 */

/**
 * 函数无法分配内存时从池接收回调的类型。处理这种情况的优雅方法是抛出异常，这是大多数库组件所期望的
 */
typedef void pj_pool_callback(pj_pool_t *pool, pj_size_t size);

/**
 * 这个类由池内部使用，它描述了一个内存块，从中可以分配用户内存分配
 */
typedef struct pj_pool_block
{
    PJ_DECL_LIST_MEMBER(struct pj_pool_block);  /**< List's prev and next.  */
    unsigned char    *buf;                      /**< 缓存区的起始地址       */
    unsigned char    *cur;                      /**< 当前分配的指针     */
    unsigned char    *end;                      /**< 缓存区的结束         */
} pj_pool_block;


/**
 * 此结构描述内存池。只有池工厂的实现者需要关心这个结构的内容
 */
struct pj_pool_t
{
    PJ_DECL_LIST_MEMBER(struct pj_pool_t);  /**< 标准列表成员    */

    /** 内存池的名称 */
    char	    obj_name[PJ_MAX_OBJ_NAME];

    /** 内存池的工厂类 */
    pj_pool_factory *factory;

    /** Data put by factory */
    void	    *factory_data;

    /** 池分配的当前容量 */
    pj_size_t	    capacity;

    /** 增量 */
    pj_size_t	    increment_size;

    /** 内存池所分配的所有内存块的列表 */
    pj_pool_block   block_list;

    /** 当池无法分配内存时要调用的回调 */
    pj_pool_callback *callback;

};


/**
 * 定义初始内存池管理数据需要的内存大小
 */
#define PJ_POOL_SIZE	        (sizeof(struct pj_pool_t))

/** 
 * 内存池对齐，必须是2的幂
 */
#ifndef PJ_POOL_ALIGNMENT
#   define PJ_POOL_ALIGNMENT    4
#endif

/**
 * 从池工厂创建新池。此包装器将调用池工厂的 create_pool 成员
 *
 * @param factory	    内存池工厂
 * @param name		    要分配给池的名称。名称不应长于 PJ_MAX_OBJ_NAME（32个字符），否则将被截断
 * @param initial_size	池使用的初始内存块的大小。
 *                      请注意，对于来自此块的管理区域，池将占用 68+20 字节
 * @param increment_size    池内存不足时要分配的每个附加块的大小。如果用户请求的内存大于此大小，则会发生错误。
 *                          注意，每次池分配额外的块时，都需要 PJ_POOL_SIZE 来存储一些管理信息
 * @param callback      当池中发生错误时要调用的回调。如果此值为 NULL，则将使用池工厂策略的回调。
 *                      请注意，在创建池期间发生错误时，不会调用回调本身。相反，将返回NULL
 *
 * @return                  The memory pool, or NULL.
 */
PJ_IDECL(pj_pool_t*) pj_pool_create(pj_pool_factory *factory, 
				    const char *name,
				    pj_size_t initial_size, 
				    pj_size_t increment_size,
				    pj_pool_callback *callback);

/**
 * Release the pool back to pool factory.
 *
 * @param pool	    Memory pool.
 */
PJ_IDECL(void) pj_pool_release( pj_pool_t *pool );


/**
 * Release the pool back to pool factory and set the pool pointer to zero.
 *
 * @param ppool	    Pointer to memory pool.
 */
PJ_IDECL(void) pj_pool_safe_release( pj_pool_t **ppool );


/**
 * Get pool object name.
 *
 * @param pool the pool.
 *
 * @return pool name as NULL terminated string.
 */
PJ_IDECL(const char *) pj_pool_getobjname( const pj_pool_t *pool );

/**
 * Reset the pool to its state when it was initialized.
 * This means that if additional blocks have been allocated during runtime, 
 * then they will be freed. Only the original block allocated during 
 * initialization is retained. This function will also reset the internal 
 * counters, such as pool capacity and used size.
 *
 * @param pool the pool.
 */
PJ_DECL(void) pj_pool_reset( pj_pool_t *pool );


/**
 * Get the pool capacity, that is, the system storage that have been allocated
 * by the pool, and have been used/will be used to allocate user requests.
 * There's no guarantee that the returned value represent a single
 * contiguous block, because the capacity may be spread in several blocks.
 *
 * @param pool	the pool.
 *
 * @return the capacity.
 */
PJ_IDECL(pj_size_t) pj_pool_get_capacity( pj_pool_t *pool );

/**
 * Get the total size of user allocation request.
 *
 * @param pool	the pool.
 *
 * @return the total size.
 */
PJ_IDECL(pj_size_t) pj_pool_get_used_size( pj_pool_t *pool );

/**
 * Allocate storage with the specified size from the pool.
 * If there's no storage available in the pool, then the pool can allocate more
 * blocks if the increment size is larger than the requested size.
 *
 * @param pool	    the pool.
 * @param size	    the requested size.
 *
 * @return pointer to the allocated memory.
 *
 * @see PJ_POOL_ALLOC_T
 */
PJ_IDECL(void*) pj_pool_alloc( pj_pool_t *pool, pj_size_t size);

/**
 * Allocate storage  from the pool, and initialize it to zero.
 * This function behaves like pj_pool_alloc(), except that the storage will
 * be initialized to zero.
 *
 * @param pool	    the pool.
 * @param count	    the number of elements in the array.
 * @param elem	    the size of individual element.
 *
 * @return pointer to the allocated memory.
 */
PJ_IDECL(void*) pj_pool_calloc( pj_pool_t *pool, pj_size_t count, 
				pj_size_t elem);


/**
 * Allocate storage from the pool and initialize it to zero.
 *
 * @param pool	    The pool.
 * @param size	    The size to be allocated.
 *
 * @return	    Pointer to the allocated memory.
 *
 * @see PJ_POOL_ZALLOC_T
 */
PJ_INLINE(void*) pj_pool_zalloc(pj_pool_t *pool, pj_size_t size)
{
    return pj_pool_calloc(pool, 1, size);
}


/**
 * This macro allocates memory from the pool and returns the instance of
 * the specified type. It provides a stricker type safety than pj_pool_alloc()
 * since the return value of this macro will be type-casted to the specified
 * type.
 *
 * @param pool	    The pool
 * @param type	    The type of object to be allocated
 *
 * @return	    Memory buffer of the specified type.
 */
#define PJ_POOL_ALLOC_T(pool,type) \
	    ((type*)pj_pool_alloc(pool, sizeof(type)))

/**
 * This macro allocates memory from the pool, zeroes the buffer, and 
 * returns the instance of the specified type. It provides a stricker type 
 * safety than pj_pool_zalloc() since the return value of this macro will be 
 * type-casted to the specified type.
 *
 * @param pool	    The pool
 * @param type	    The type of object to be allocated
 *
 * @return	    Memory buffer of the specified type.
 */
#define PJ_POOL_ZALLOC_T(pool,type) \
	    ((type*)pj_pool_zalloc(pool, sizeof(type)))

/*
 * Internal functions
 */
PJ_IDECL(void*) pj_pool_alloc_from_block(pj_pool_block *block, pj_size_t size);
PJ_DECL(void*) pj_pool_allocate_find(pj_pool_t *pool, pj_size_t size);


	
/**
 * @}	// PJ_POOL
 */

/* **************************************************************************/
/**
 * @defgroup PJ_POOL_FACTORY Pool Factory and Policy
 * @ingroup PJ_POOL_GROUP
 * @brief
 * A pool object must be created through a factory. A factory not only provides
 * generic interface functions to create and release pool, but also provides 
 * strategy to manage the life time of pools. One sample implementation, 
 * \a pj_caching_pool, can be set to keep the pools released by application for
 * future use as long as the total memory is below the limit.
 * 
 * The pool factory interface declared in PJLIB is designed to be extensible.
 * Application can define its own strategy by creating it's own pool factory
 * implementation, and this strategy can be used even by existing library
 * without recompilation.
 *
 * \section PJ_POOL_FACTORY_ITF Pool Factory Interface
 * The pool factory defines the following interface:
 *  - \a policy: the memory pool factory policy.
 *  - \a create_pool(): create a new memory pool.
 *  - \a release_pool(): release memory pool back to factory.
 *
 * \section PJ_POOL_FACTORY_POL Pool Factory Policy.
 *
 * A pool factory only defines functions to create and release pool and how
 * to manage pools, but the rest of the functionalities are controlled by
 * policy. A pool policy defines:
 *  - how memory block is allocated and deallocated (the default implementation
 *    allocates and deallocate memory by calling malloc() and free()).
 *  - callback to be called when memory allocation inside a pool fails (the
 *    default implementation will throw PJ_NO_MEMORY_EXCEPTION exception).
 *  - concurrency when creating and releasing pool from/to the factory.
 *
 * A pool factory can be given different policy during creation to make
 * it behave differently. For example, caching pool factory can be configured
 * to allocate and deallocate from a static/contiguous/preallocated memory 
 * instead of using malloc()/free().
 * 
 * What strategy/factory and what policy to use is not defined by PJLIB, but
 * instead is left to application to make use whichever is most efficient for
 * itself.
 *
 * The pool factory policy controls the behaviour of memory factories, and
 * defines the following interface:
 *  - \a block_alloc(): allocate memory block from backend memory mgmt/system.
 *  - \a block_free(): free memory block back to backend memory mgmt/system.
 * @{
 */

/* We unfortunately don't have support for factory policy options as now,
   so we keep this commented at the moment.
enum PJ_POOL_FACTORY_OPTION
{
    PJ_POOL_FACTORY_SERIALIZE = 1
};
*/

/**
 * This structure declares pool factory interface.
 */
typedef struct pj_pool_factory_policy
{
    /**
     * Allocate memory block (for use by pool). This function is called
     * by memory pool to allocate memory block.
     * 
     * @param factory	Pool factory.
     * @param size	The size of memory block to allocate.
     *
     * @return		Memory block.
     */
    void* (*block_alloc)(pj_pool_factory *factory, pj_size_t size);

    /**
     * Free memory block.
     *
     * @param factory	Pool factory.
     * @param mem	Memory block previously allocated by block_alloc().
     * @param size	The size of memory block.
     */
    void (*block_free)(pj_pool_factory *factory, void *mem, pj_size_t size);

    /**
     * Default callback to be called when memory allocation fails.
     */
    pj_pool_callback *callback;

    /**
     * Option flags.
     */
    unsigned flags;

} pj_pool_factory_policy;

/**
 * \def PJ_NO_MEMORY_EXCEPTION
 * This constant denotes the exception number that will be thrown by default
 * memory factory policy when memory allocation fails.
 *
 * @see pj_NO_MEMORY_EXCEPTION()
 */
PJ_DECL_DATA(int) PJ_NO_MEMORY_EXCEPTION;

/**
 * Get #PJ_NO_MEMORY_EXCEPTION constant.
 */ 
PJ_DECL(int) pj_NO_MEMORY_EXCEPTION(void);

/**
 * This global variable points to default memory pool factory policy.
 * The behaviour of the default policy is:
 *  - block allocation and deallocation use malloc() and free().
 *  - callback will raise PJ_NO_MEMORY_EXCEPTION exception.
 *  - access to pool factory is not serialized (i.e. not thread safe).
 *
 * @see pj_pool_factory_get_default_policy
 */
PJ_DECL_DATA(pj_pool_factory_policy) pj_pool_factory_default_policy;


/**
 * Get the default pool factory policy.
 *
 * @return the pool policy.
 */
PJ_DECL(const pj_pool_factory_policy*) pj_pool_factory_get_default_policy(void);


/**
 * This structure contains the declaration for pool factory interface.
 */
struct pj_pool_factory
{
    /**
     * Memory pool policy.
     */
    pj_pool_factory_policy policy;

    /**
    * Create a new pool from the pool factory.
    *
    * @param factory	The pool factory.
    * @param name	the name to be assigned to the pool. The name should 
    *			not be longer than PJ_MAX_OBJ_NAME (32 chars), or 
    *			otherwise it will be truncated.
    * @param initial_size the size of initial memory blocks taken by the pool.
    *			Note that the pool will take 68+20 bytes for 
    *			administrative area from this block.
    * @param increment_size the size of each additional blocks to be allocated
    *			when the pool is running out of memory. If user 
    *			requests memory which is larger than this size, then 
    *			an error occurs.
    *			Note that each time a pool allocates additional block, 
    *			it needs 20 bytes (equal to sizeof(pj_pool_block)) to 
    *			store some administrative info.
    * @param callback	Cllback to be called when error occurs in the pool.
    *			Note that when an error occurs during pool creation, 
    *			the callback itself is not called. Instead, NULL 
    *			will be returned.
    *
    * @return the memory pool, or NULL.
    */
    pj_pool_t*	(*create_pool)( pj_pool_factory *factory,
				const char *name,
				pj_size_t initial_size, 
				pj_size_t increment_size,
				pj_pool_callback *callback);

    /**
     * Release the pool to the pool factory.
     *
     * @param factory	The pool factory.
     * @param pool	The pool to be released.
    */
    void (*release_pool)( pj_pool_factory *factory, pj_pool_t *pool );

    /**
     * Dump pool status to log.
     *
     * @param factory	The pool factory.
     */
    void (*dump_status)( pj_pool_factory *factory, pj_bool_t detail );

    /**
     * This is optional callback to be called by allocation policy when
     * it allocates a new memory block. The factory may use this callback
     * for example to keep track of the total number of memory blocks
     * currently allocated by applications.
     *
     * @param factory	    The pool factory.
     * @param size	    Size requested by application.
     *
     * @return		    MUST return PJ_TRUE, otherwise the block
     *                      allocation is cancelled.
     */
    pj_bool_t (*on_block_alloc)(pj_pool_factory *factory, pj_size_t size);

    /**
     * This is optional callback to be called by allocation policy when
     * it frees memory block. The factory may use this callback
     * for example to keep track of the total number of memory blocks
     * currently allocated by applications.
     *
     * @param factory	    The pool factory.
     * @param size	    Size freed.
     */
    void (*on_block_free)(pj_pool_factory *factory, pj_size_t size);

};

/**
 * This function is intended to be used by pool factory implementors.
 * @param factory           Pool factory.
 * @param name              Pool name.
 * @param initial_size      Initial size.
 * @param increment_size    Increment size.
 * @param callback          Callback.
 * @return                  The pool object, or NULL.
 */
PJ_DECL(pj_pool_t*) pj_pool_create_int(	pj_pool_factory *factory, 
					const char *name,
					pj_size_t initial_size, 
					pj_size_t increment_size,
					pj_pool_callback *callback);

/**
 * This function is intended to be used by pool factory implementors.
 * @param pool              The pool.
 * @param name              Pool name.
 * @param increment_size    Increment size.
 * @param callback          Callback function.
 */
PJ_DECL(void) pj_pool_init_int( pj_pool_t *pool, 
				const char *name,
				pj_size_t increment_size,
				pj_pool_callback *callback);

/**
 * This function is intended to be used by pool factory implementors.
 * @param pool      The memory pool.
 */
PJ_DECL(void) pj_pool_destroy_int( pj_pool_t *pool );


/**
 * Dump pool factory state.
 * @param pf	    The pool factory.
 * @param detail    Detail state required.
 */
PJ_INLINE(void) pj_pool_factory_dump( pj_pool_factory *pf,
				      pj_bool_t detail )
{
    (*pf->dump_status)(pf, detail);
}

/**
 *  @}	// PJ_POOL_FACTORY
 */

/* **************************************************************************/

/**
 * @defgroup PJ_CACHING_POOL Caching Pool Factory
 * @ingroup PJ_POOL_GROUP
 * @brief
 * Caching pool is one sample implementation of pool factory where the
 * factory can reuse memory to create a pool. Application defines what the 
 * maximum memory the factory can hold, and when a pool is released the
 * factory decides whether to destroy the pool or to keep it for future use.
 * If the total amount of memory in the internal cache is still within the
 * limit, the factory will keep the pool in the internal cache, otherwise the
 * pool will be destroyed, thus releasing the memory back to the system.
 *
 * @{
 */

/**
 * Number of unique sizes, to be used as index to the free list.
 * Each pool in the free list is organized by it's size.
 */
#define PJ_CACHING_POOL_ARRAY_SIZE	16

/**
 * Declaration for caching pool. Application doesn't normally need to
 * care about the contents of this struct, it is only provided here because
 * application need to define an instance of this struct (we can not allocate
 * the struct from a pool since there is no pool factory yet!).
 */
struct pj_caching_pool 
{
    /** Pool factory interface, must be declared first. */
    pj_pool_factory factory;

    /** Current factory's capacity, i.e. number of bytes that are allocated
     *  and available for application in this factory. The factory's
     *  capacity represents the size of all pools kept by this factory
     *  in it's free list, which will be returned to application when it
     *  requests to create a new pool.
     */
    pj_size_t	    capacity;

    /** Maximum size that can be held by this factory. Once the capacity
     *  has exceeded @a max_capacity, further #pj_pool_release() will
     *  flush the pool. If the capacity is still below the @a max_capacity,
     *  #pj_pool_release() will save the pool to the factory's free list.
     */
    pj_size_t       max_capacity;

    /**
     * Number of pools currently held by applications. This number gets
     * incremented everytime #pj_pool_create() is called, and gets
     * decremented when #pj_pool_release() is called.
     */
    pj_size_t       used_count;

    /**
     * Total size of memory currently used by application.
     */
    pj_size_t	    used_size;

    /**
     * The maximum size of memory used by application throughout the life
     * of the caching pool.
     */
    pj_size_t	    peak_used_size;

    /**
     * Lists of pools in the cache, indexed by pool size.
     */
    pj_list	    free_list[PJ_CACHING_POOL_ARRAY_SIZE];

    /**
     * List of pools currently allocated by applications.
     */
    pj_list	    used_list;

    /**
     * Internal pool.
     */
    char	    pool_buf[256 * (sizeof(size_t) / 4)];

    /**
     * Mutex.
     */
    pj_lock_t	   *lock;
};



/**
 * Initialize caching pool.
 *
 * @param ch_pool	The caching pool factory to be initialized.
 * @param policy	Pool factory policy.
 * @param max_capacity	The total capacity to be retained in the cache. When
 *			the pool is returned to the cache, it will be kept in
 *			recycling list if the total capacity of pools in this
 *			list plus the capacity of the pool is still below this
 *			value.
 */
PJ_DECL(void) pj_caching_pool_init( pj_caching_pool *ch_pool, 
				    const pj_pool_factory_policy *policy,
				    pj_size_t max_capacity);


/**
 * Destroy caching pool, and release all the pools in the recycling list.
 *
 * @param ch_pool	The caching pool.
 */
PJ_DECL(void) pj_caching_pool_destroy( pj_caching_pool *ch_pool );

/**
 * @}	// PJ_CACHING_POOL
 */

#  if PJ_FUNCTIONS_ARE_INLINED
#    include "pool_i.h"
#  endif

PJ_END_DECL
    
#endif	/* __PJ_POOL_H__ */

