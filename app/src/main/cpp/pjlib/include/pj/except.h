/**
 * 已完成：
 * 	C 语言异常处理
 * 		PJ_USE_EXCEPTION;
 *
 * 		PJ_TRY{
 *
 * 		}PJ_CATCH(expression){
 *
 * 		}PJ_END;
 *
 * 	Liunx  实际上使用的是  setjmp 和 longjmp
 */
#ifndef __PJ_EXCEPTION_H__
#define __PJ_EXCEPTION_H__

/**
 * @file except.h
 * @brief C 异常处理
 */

#include <pj/types.h>
#include <pj/compat/setjmp.h>
#include <pj/log.h>


PJ_BEGIN_DECL


/**
 * @defgroup PJ_EXCEPT 异常处理
 * @ingroup PJ_MISC
 * @{
 *
 * \section pj_except_sample_sec 快速示例
 *
 * 快速集成的例子：
 *  - @ref page_pjlib_samples_except_c
 *  - @ref page_pjlib_exception_test
 *
 * \section pj_except_except 异常处理
 *
 * 这个模块在C语言中提供了类似于C++的语法异常处理。在Win32系统中，如果宏 PJ_EXCEPTION_USE_WIN32_SEH 为非零，
 * 则使用 Windows结构化异常处理（SEH）。否则它将使用 setjmp() 和longjmp()
 *
 * 在一些 setjmp/longjmp 不可用的平台上，提供了 setjmp/longjmp 实现。
 * 有关兼容性，请参见<pj/compat/setjmp.h>
 *
 * 异常处理机制是完全线程安全的，因此一个线程引发的异常不会干扰其他线程
 *
 * 异常处理构造类似于 C++。模块的构造类似于以下示例：
 *
 * \verbatim
   #define NO_MEMORY     1
   #define SYNTAX_ERROR  2
  
   int sample1()
   {
      PJ_USE_EXCEPTION;  // 声明本地异常堆栈
  
      PJ_TRY {
        ...// do something..
      }
      PJ_CATCH(NO_MEMORY) {
        ... // handle exception 1
      }
      PJ_END;
   }

   int sample2()
   {
      PJ_USE_EXCEPTION;  // 声明本地异常堆栈
  
      PJ_TRY {
        ...// do something..
      }
      PJ_CATCH_ANY {
         if (PJ_GET_EXCEPTION() == NO_MEMORY)
	    ...; // 没有内存
	 else if (PJ_GET_EXCEPTION() == SYNTAX_ERROR)
	    ...; // 语法错误
      }
      PJ_END;
   }
   \endverbatim
 *
 * 上面的示例使用硬编码的异常ID。强烈建议应用程序请求唯一的异常ID，而不是像上面那样的硬编码值
 *
 * \section pj_except_reg 异常ID 分配
 *
 * 为了确保一致地使用异常ID（number）并防止应用程序中的ID冲突，强烈建议应用程序为每个可能的异常类型分配一个异常ID。
 * 作为此过程的额外功能，应用程序可以在抛出特定异常时标识异常的名称
 *
 * 使用以下API执行异常ID管理：
 *  - pj_exception_id_alloc().
 *  - pj_exception_id_free().
 *  - pj_exception_id_name().
 *
 *
 * PJLIB 本身自动分配一个异常id，即在 <pj/pool.h>中声明的 PJ_NO_MEMORY_EXCEPTION。
 * 默认池策略在分配内存失败时引发此异常ID
 *
 * 注意事项：
 * 		与C++异常不同，如果抛出异常，这里的方案不会调用局部对象的析构函数。当一个函数拥有一些资源，如池或互斥等时，必须小心。
		-如果不使用嵌套的 PJ_USE_EXCEPTION，就不能在单个函数中生成嵌套异常。样品：

  \verbatim
	void wrong_sample()
	{
	    PJ_USE_EXCEPTION;

	    PJ_TRY {
		// Do stuffs
		...
	    }
	    PJ_CATCH_ANY {
		// Do other stuffs
		....
		..

 		//下面的方块是错误的！必须在此块中再次声明PJ_USE_EXCEPTION
		PJ_TRY {
		    ..
		}
		PJ_CATCH_ANY {
		    ..
		}
		PJ_END;
	    }
	    PJ_END;
	}

  \endverbatim

 	不能退出 PJ_TRY 块中的函数。正确的方法是在执行PJ_END 之后从函数返回。
 	例如，以下代码将不会在该代码中产生崩溃，而是在PJ_TRY 块的后续执行中产生崩溃：
  \verbatim
        void wrong_sample()
	{
	    PJ_USE_EXCEPTION;

	    PJ_TRY {
		// do some stuffs
		...
		return;	        <======= DO NOT DO THIS!
	    }
	    PJ_CATCH_ANY {
	    }
	    PJ_END;
	}
  \endverbatim
  
 *  - 您不能提供超过 PJ_CATCH 或 PJ_CATCH_ANY，也不能将PJ_CATCH和 PJ_CATCH_ANY 用于一次 PJ_TRY尝试
 *  - 异常将总是由第一个处理程序捕获（与C++不同，只有类型匹配时才会捕获异常）

 * \section PJ_EX_KEYWORDS 关键字
 *
 * \subsection PJ_THROW PJ_THROW(expression)
 * 抛出异常。抛出的表达式是作为表达式结果的整数。这个关键字可以在程序的任何地方指定
 *
 * \subsection PJ_USE_EXCEPTION PJ_USE_EXCEPTION
 * 在函数块（或任何块）的变量定义部分中指定此项，以指定该块具有 PJ_TRY/PJ_CATCH 异常块。
 * 实际上，这只是一个声明局部变量的宏，用于将异常状态推送到异常堆栈。
 *
 * 注意：您必须指定 PJ_USE_EXCEPTION 作为局部变量声明中的最后一条语句，因为它的计算结果可能为 nothing
 *
 * \subsection PJ_TRY PJ_TRY
 * PJ_TRY 关键字后面通常跟一个块。如果此块中抛出异常，则执行将恢复到 PJ_CATCH 处理程序
 *
 * \subsection PJ_CATCH PJ_CATCH(expression)
 * PJ_CATCH 通常有一个方块。如果抛出的异常等于 PJ_CATCH 中指定的表达式，则将执行此块
 *
 * \subsection PJ_CATCH_ANY PJ_CATCH_ANY
 * PJ_CATCH 后面通常跟一个块。如果 TRY 块中出现任何异常，将执行此块。
 *
 * \subsection PJ_END PJ_END
 * 指定此关键字以标记 PJ_TRY/PJ_CATCH 块的结束
 *
 * \subsection PJ_GET_EXCEPTION PJ_GET_EXCEPTION(void)
 * 获取引发的最后一个异常。此宏通常在 PJ_CATCH 或 PJ_CATCH_ANY 块内调用，尽管它可以在PJ_USE_EXCEPTION 定义所在的任何位置使用
 *
 * 
 * \section pj_except_examples_sec 例子
 *
 * 有关如何使用异常构造的一些示例，请参见：
 *  - @ref page_pjlib_samples_except_c
 *  - @ref page_pjlib_exception_test
 */

/**
 * 分配唯一的异常id
 * 在使用异常构造之前，应用程序不必分配唯一的异常ID。但是，这样做可以确保异常ID没有冲突。
 * 另外，当通过此函数获取异常编号时，库可以为异常分配名称（仅当 PJ_HAS_EXCEPTION_NAMES 处于启用状态（默认值为yes））并在捕获异常时找到异常名称
 *
 * @param name      要与异常ID关联的名称
 * @param id        接收ID的指针
 *
 * @return          成功返回 PJ_SUCCESS，Id 耗尽则返回 PJ_ETOOMANY
 */
PJ_DECL(pj_status_t) pj_exception_id_alloc(const char *name,
                                           pj_exception_id_t *id);

/**
 * 释放异常ID
 *
 * @param id        异常ID
 *
 * @return          成功返回 PJ_SUCCESS，否则返回相应错误码
 */
PJ_DECL(pj_status_t) pj_exception_id_free(pj_exception_id_t id);

/**
 * 检索与异常id关联的名称
 *
 * @param id        异常ID
 *
 * @return          与指定ID关联的名称
 */
PJ_DECL(const char*) pj_exception_id_name(pj_exception_id_t id);


/** @} */

#if defined(PJ_EXCEPTION_USE_WIN32_SEH) && PJ_EXCEPTION_USE_WIN32_SEH != 0
/*****************************************************************************
 **
 ** IMPLEMENTATION OF EXCEPTION USING WINDOWS SEH
 **
 ****************************************************************************/
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

PJ_IDECL_NO_RETURN(void)
pj_throw_exception_(pj_exception_id_t id) PJ_ATTR_NORETURN
{
    RaiseException(id,1,0,NULL);
}

#define PJ_USE_EXCEPTION    
#define PJ_TRY		    __try
#define PJ_CATCH(id)	    __except(GetExceptionCode()==id ? \
				      EXCEPTION_EXECUTE_HANDLER : \
				      EXCEPTION_CONTINUE_SEARCH)
#define PJ_CATCH_ANY	    __except(EXCEPTION_EXECUTE_HANDLER)
#define PJ_END		    
#define PJ_THROW(id)	    pj_throw_exception_(id)
#define PJ_GET_EXCEPTION()  GetExceptionCode()


#elif defined(PJ_SYMBIAN) && PJ_SYMBIAN!=0
/*****************************************************************************
 **
 ** IMPLEMENTATION OF EXCEPTION USING SYMBIAN LEAVE/TRAP FRAMEWORK
 **
 ****************************************************************************/

/* To include this file, the source file must be compiled as
 * C++ code!
 */
#ifdef __cplusplus

class TPjException
{
public:
    int code_;
};

#define PJ_USE_EXCEPTION
#define PJ_TRY			try
//#define PJ_CATCH(id)		
#define PJ_CATCH_ANY		catch (const TPjException & pj_excp_)
#define PJ_END
#define PJ_THROW(x_id)		do { TPjException e; e.code_=x_id; throw e;} \
				while (0)
#define PJ_GET_EXCEPTION()	pj_excp_.code_

#else

#define PJ_USE_EXCEPTION
#define PJ_TRY				
#define PJ_CATCH_ANY		if (0)
#define PJ_END
#define PJ_THROW(x_id)		do { PJ_LOG(1,("PJ_THROW"," error code = %d",x_id)); } while (0)
#define PJ_GET_EXCEPTION()	0

#endif	/* __cplusplus */

#else
/*****************************************************************************
 **
 ** 使用通用的 setjmp /longjmp 来实现异常处理
 **
 ****************************************************************************/

/**
 * 这个结构（用户应该看不见）管理TRY处理程序堆栈
 */
struct pj_exception_state_t
{    
    pj_jmp_buf state;                   /**< jmp_buf.                    */
    struct pj_exception_state_t *prev;  /**< Previous state in the list. */
};

/**
 * 抛出异常
 * @param id    异常ID
 */
PJ_DECL_NO_RETURN(void) 
pj_throw_exception_(pj_exception_id_t id) PJ_ATTR_NORETURN;

/**
 * 推送异常处理程序
 */
PJ_DECL(void) pj_push_exception_handler_(struct pj_exception_state_t *rec);

/**
 * 弹出异常处理程序
 */
PJ_DECL(void) pj_pop_exception_handler_(struct pj_exception_state_t *rec);

/**
 * 声明函数将使用 exception
 * @hideinitializer
 */
#define PJ_USE_EXCEPTION    struct pj_exception_state_t pj_x_except__; int pj_x_code__

/**
 * 启动异常规范块
 * @hideinitializer
 */
#define PJ_TRY		    if (1) { \
				pj_push_exception_handler_(&pj_x_except__); \
				pj_x_code__ = pj_setjmp(pj_x_except__.state); \
				if (pj_x_code__ == 0)
/**
 * 捕获指定的异常Id
 * @param id    捕获的异常 Id
 * @hideinitializer
 */
#define PJ_CATCH(id)	    else if (pj_x_code__ == (id))

/**
 * 捕获任何异常id
 * @hideinitializer
 */
#define PJ_CATCH_ANY	    else

/**
 * 异常规范块结束
 * @hideinitializer
 */
#define PJ_END			pj_pop_exception_handler_(&pj_x_except__); \
			    } else {}

/**
 * 抛出异常
 * @param exception_id  异常id
 * @hideinitializer
 */
#define PJ_THROW(exception_id)	pj_throw_exception_(exception_id)

/**
 * 获取当前异常
 * @return      当前异常的 code
 * @hideinitializer
 */
#define PJ_GET_EXCEPTION()	(pj_x_code__)

#endif	/* PJ_EXCEPTION_USE_WIN32_SEH */


PJ_END_DECL



#endif	/* __PJ_EXCEPTION_H__ */


