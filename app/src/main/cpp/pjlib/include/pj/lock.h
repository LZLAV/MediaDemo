/**
 * 已完成
 *      锁的高级抽象
 */
#ifndef __PJ_LOCK_H__
#define __PJ_LOCK_H__

/**
 * @file lock.h
 * @brief 锁的高级抽象
 */
#include <pj/types.h>

PJ_BEGIN_DECL

/**
 * @defgroup PJ_LOCK 锁对象
 * @ingroup PJ_OS
 * @{
 *
 * 锁对象是对不同锁机制的更高抽象，它提供相同的API来操作不同的锁类型
 * （例如@ref PJ_MUTEX "mutex"、@ref PJ_SEM "semaphores"或空锁）。
 * 因为锁对象对于不同类型的锁实现具有相同的API，所以可以在函数参数中传递。因此，它可以用于控制特定功能的锁定策略
 *
 */


/**
 * 创建简单的非递归互斥锁对象
 *
 * @param pool	    内存池
 * @param name	    锁对象名称
 * @param lock	    存储返回锁句柄的指针
 *
 * @return	    成功返回PJ_SUCCESS，否则返回相应的错误码
 */
PJ_DECL(pj_status_t) pj_lock_create_simple_mutex( pj_pool_t *pool,
						  const char *name,
						  pj_lock_t **lock );

/**
 * 创建一个递归锁
 *
 * @param pool	    内存池
 * @param name	    锁对象的名称
 * @param lock	    存储返回锁句柄的指针
 *
 * @return	    成功返回PJ_SUCCESS，否则返回相应的错误码
 */
PJ_DECL(pj_status_t) pj_lock_create_recursive_mutex( pj_pool_t *pool,
						     const char *name,
						     pj_lock_t **lock );


/**
 * 创建空互斥体。空互斥体实际上没有任何同步对象附加到它
 *
 * @param pool	    内存池
 * @param name	    锁对象的名称
 * @param lock	    存储返回锁句柄的指针
 *
 * @return	    成功返回 PJ_SUCCESS，否则返回相应的错误码
 */
PJ_DECL(pj_status_t) pj_lock_create_null_mutex( pj_pool_t *pool,
						const char *name,
						pj_lock_t **lock );


#if defined(PJ_HAS_SEMAPHORE) && PJ_HAS_SEMAPHORE != 0
/**
 * 创建信号量锁对象
 *
 * @param pool	    内存池
 * @param name	    锁对象的名称
 * @param initial   信号量的初始值
 * @param max	    信号量的最大值
 * @param lock	    存储返回锁句柄的指针
 *
 * @return	    成功返回 PJ_SUCCESS，否则返回相应的错误码
 */
PJ_DECL(pj_status_t) pj_lock_create_semaphore( pj_pool_t *pool,
					       const char *name,
					       unsigned initial,
					       unsigned max,
					       pj_lock_t **lock );

#endif	/* PJ_HAS_SEMAPHORE */

/**
 * 获取指定锁对象上的锁
 *
 * @param lock	    锁对象
 *
 * @return	    成功返回 PJ_SUCCESS，否则返回相应的错误码
 */
PJ_DECL(pj_status_t) pj_lock_acquire( pj_lock_t *lock );


/**
 * 尝试获取指定锁对象上的锁
 *
 * @param lock	    锁对象
 *
 * @return	    成功返回 PJ_SUCCESS，否则返回相应的错误码
 */
PJ_DECL(pj_status_t) pj_lock_tryacquire( pj_lock_t *lock );


/**
 * 释放指定对象上的锁
 *
 * @param lock	    锁对象
 *
 * @return	    成功返回 PJ_SUCCESS，否则返回相应的错误码
 */
PJ_DECL(pj_status_t) pj_lock_release( pj_lock_t *lock );


/**
 * 销毁锁对象
 *
 * @param lock	    锁对象
 *
 * @return	    成功返回 PJ_SUCCESS，否则返回相应的错误码
 */
PJ_DECL(pj_status_t) pj_lock_destroy( pj_lock_t *lock );


/** @} */


/**
 * @defgroup PJ_GRP_LOCK 组锁
 * @ingroup PJ_LOCK
 * @{
 *
 * 组锁是一个同步对象，用于管理同一逻辑组中成员之间的并发性。这类群体的例子有：
 * 	-对话框，其成员包括对话框本身、邀请会话和多个事务
 * 	-ICE，其成员包括 ICE流传输、ICE会话、STUN 套接字、TURN 套接字和向下到ioqueue key
 * 	组锁有三个功能：
 * 		-互斥：防止多个线程同时访问资源
 * 		-会话管理：确保资源在其他人仍在使用或即将使用时不会被销毁
 * 		-锁协调器：在多个锁对象之间提供统一的锁顺序，这是避免死锁所必需的
 *
 * 	组锁的要求是：
 * 		-必须满足上述所有功能
 * 		-必须允许成员加入或离开组（例如，可以在对话框中添加或删除事务）
 * 		-必须能够与外部锁同步（例如，对话框锁必须能够与 PJSUA 锁同步）
 *
 * 更多信息详见： https://trac.pjsip.org/repos/wiki/Group_Lock
 */

/**
 * 创建组锁的设置
 */
typedef struct pj_grp_lock_config
{
    /**
     * 创建标识，当前必须为零
     */
    unsigned	flags;

} pj_grp_lock_config;


/**
 * 使用默认值初始化配置
 *
 * @param cfg		要初始化的配置
 */
PJ_DECL(void) pj_grp_lock_config_default(pj_grp_lock_config *cfg);

/**
 * 创建组锁对象。最初，组锁的引用计数器为1
 *
 * @param pool		组锁只使用 pool参数来获取池工厂，它将从中创建自己的池
 * @param cfg		可选配置
 * @param p_grp_lock	接收新创建的组锁的指针
 *
 * @return		成功返回 PJ_SUCCESS，否则返回相应错误码
 */
PJ_DECL(pj_status_t) pj_grp_lock_create(pj_pool_t *pool,
                                        const pj_grp_lock_config *cfg,
                                        pj_grp_lock_t **p_grp_lock);

/**
 * 使用指定的析构函数处理程序创建一个组锁对象，在组锁即将被销毁时由组锁调用。最初，组锁的引用计数器为1
 *
 * @param pool		组锁只使用 pool参数来获取池工厂，它将从中创建自己的池
 * @param cfg		可选配置
 * @param member	要传递给处理程序的指针
 * @param handler	销毁的处理函数
 * @param p_grp_lock	接收新创建的组锁的指针
 *
 * @return		成功返回 PJ_SUCCESS，否则返回相应错误码
 */
PJ_DECL(pj_status_t) pj_grp_lock_create_w_handler(pj_pool_t *pool,
                                        	  const pj_grp_lock_config *cfg,
                                        	  void *member,
                                                  void (*handler)(void *member),
                                        	  pj_grp_lock_t **p_grp_lock);

/**
 * 强制销毁组锁，忽略引用计数器值
 *
 * @param grp_lock	组锁
 *
 * @return		成功返回 PJ_SUCCESS，否则返回相应错误码
 */
PJ_DECL(pj_status_t) pj_grp_lock_destroy( pj_grp_lock_t *grp_lock);

/**
 * 将旧锁的内容移动到新锁并销毁旧锁
 *
 * @param old_lock	要销毁的旧组锁
 * @param new_lock	新的组锁
 *
 * @return		成功返回 PJ_SUCCESS，否则返回相应错误码
 */
PJ_DECL(pj_status_t) pj_grp_lock_replace(pj_grp_lock_t *old_lock,
                                         pj_grp_lock_t *new_lock);

/**
 * 获取指定组锁上的锁
 *
 * @param grp_lock	组锁
 *
 * @return		成功返回 PJ_SUCCESS，否则返回相应错误码
 */
PJ_DECL(pj_status_t) pj_grp_lock_acquire( pj_grp_lock_t *grp_lock);

/**
 * 获取指定组锁上的锁（如果可用），否则立即返回而不等待
 *
 * @param grp_lock	组锁
 *
 * @return		成功返回 PJ_SUCCESS，否则返回相应错误码
 */
PJ_DECL(pj_status_t) pj_grp_lock_tryacquire( pj_grp_lock_t *grp_lock);

/**
 * 释放先前持有的锁。如果组锁是最后一个持有引用计数器的组锁，则可能会导致组锁被销毁。在这种情况下，函数将返回PJ_EGONE
 *
 * @param grp_lock	组锁
 *
 * @return		成功返回 PJ_SUCCESS，否则返回相应错误码
 */
PJ_DECL(pj_status_t) pj_grp_lock_release( pj_grp_lock_t *grp_lock);

/**
 * 添加一个析构函数处理程序，在组锁即将被销毁时由组锁调用
 *
 * @param grp_lock	组锁
 * @param pool		池为处理程序分配内存
 * @param member	要传递给处理程序的指针
 * @param handler	销毁的处理函数
 *
 * @return		成功返回 PJ_SUCCESS，否则返回相应错误码
 */
PJ_DECL(pj_status_t) pj_grp_lock_add_handler(pj_grp_lock_t *grp_lock,
                                             pj_pool_t *pool,
                                             void *member,
                                             void (*handler)(void *member));

/**
 * 删除以前注册的处理程序。所有参数必须与添加处理程序时相同
 *
 * @param grp_lock	组锁
 * @param member	要传递给处理程序的指针
 * @param handler	销毁的处理函数
 *
 * @return		成功返回 PJ_SUCCESS，否则返回相应错误码
 */
PJ_DECL(pj_status_t) pj_grp_lock_del_handler(pj_grp_lock_t *grp_lock,
                                             void *member,
                                             void (*handler)(void *member));

/**
 * 增加参考计数器，以防止组锁被销毁
 *
 * @param grp_lock	组锁
 *
 * @return		成功返回 PJ_SUCCESS，否则返回相应错误码
 */
#if !PJ_GRP_LOCK_DEBUG
PJ_DECL(pj_status_t) pj_grp_lock_add_ref(pj_grp_lock_t *grp_lock);

#define pj_grp_lock_add_ref_dbg(grp_lock, x, y) pj_grp_lock_add_ref(grp_lock)

#else

#define pj_grp_lock_add_ref(g)	pj_grp_lock_add_ref_dbg(g, __FILE__, __LINE__)

PJ_DECL(pj_status_t) pj_grp_lock_add_ref_dbg(pj_grp_lock_t *grp_lock,
                                             const char *file,
                                             int line);
#endif

/**
 * 减少参考计数器。当计数器值达到零时，将销毁组锁并调用所有析构函数处理程序
 *
 * @param grp_lock	组锁
 *
 * @return		成功返回 PJ_SUCCESS，否则返回相应错误码
 */
#if !PJ_GRP_LOCK_DEBUG
PJ_DECL(pj_status_t) pj_grp_lock_dec_ref(pj_grp_lock_t *grp_lock);

#define pj_grp_lock_dec_ref_dbg(grp_lock, x, y) pj_grp_lock_dec_ref(grp_lock)
#else

#define pj_grp_lock_dec_ref(g)	pj_grp_lock_dec_ref_dbg(g, __FILE__, __LINE__)

PJ_DECL(pj_status_t) pj_grp_lock_dec_ref_dbg(pj_grp_lock_t *grp_lock,
                                             const char *file,
                                             int line);

#endif

/**
 * 获取当前引用计数值。这通常仅用于调试目的
 *
 * @param grp_lock	组锁
 *
 * @return		引用数值
 */
PJ_DECL(int) pj_grp_lock_get_ref(pj_grp_lock_t *grp_lock);


/**
 * 转储组锁信息以进行调试。如果启用了组锁调试（通过 PJ_GRP_LOCK_DEBUG）宏，这将打印组锁引用计数器值以及源文件和行。
 * 如果禁用调试，这将只打印引用计数器
 *
 * @param grp_lock	组锁
 */
PJ_DECL(void) pj_grp_lock_dump(pj_grp_lock_t *grp_lock);


/**
 * 将外部锁与组锁同步，方法是在获取组锁时将其添加到组锁要获取的锁列表中。
 * 'pos' 参数指定锁顺序，以及相对于组锁的锁顺序的相对位置。
 * 'pos' 值较低的锁将首先锁定，值为负值的锁将在组锁之前锁定（组锁的 'pos' 值为零）
 *
 * @param grp_lock	组锁
 * @param ext_lock	外部锁
 * @param pos		位置
 *
 * @return		成功返回 PJ_SUCCESS，否则返回相应错误码
 */
PJ_DECL(pj_status_t) pj_grp_lock_chain_lock(pj_grp_lock_t *grp_lock,
                                            pj_lock_t *ext_lock,
                                            int pos);

/**
 * 从组锁的同步锁列表中删除外部锁
 *
 * @param grp_lock	组锁
 * @param ext_lock	外部锁
 *
 * @return		成功返回 PJ_SUCCESS，否则返回相应错误码
 */
PJ_DECL(pj_status_t) pj_grp_lock_unchain_lock(pj_grp_lock_t *grp_lock,
                                              pj_lock_t *ext_lock);


/** @} */


PJ_END_DECL


#endif	/* __PJ_LOCK_H__ */

