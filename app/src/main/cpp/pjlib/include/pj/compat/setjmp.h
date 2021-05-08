/**
 * 已完成
 *  goto 仅限于函数内跳转，不支持函数间跳转
 *
 *  函数间跳转
 *      setjmp()    设置跳转点
 *      longjmp(jb_buf buf,int i)   跳转到跳转点，并且携带返回值 i
 *
 *  常见于 C 语言异常处理
 */
#ifndef __PJ_COMPAT_SETJMP_H__
#define __PJ_COMPAT_SETJMP_H__

/**
 * @file setjmp.h
 * @brief Provides setjmp.h functionality.
 */

#if defined(PJ_HAS_SETJMP_H) && PJ_HAS_SETJMP_H != 0
#  include <setjmp.h>
   typedef jmp_buf pj_jmp_buf;
#  ifndef pj_setjmp
#    define pj_setjmp(buf)	setjmp(buf)
#  endif
#  ifndef pj_longjmp
#    define pj_longjmp(buf,d)	longjmp(buf,d)
#  endif

#elif defined(PJ_SYMBIAN) && PJ_SYMBIAN!=0
    /* Symbian framework don't use setjmp/longjmp */
    
#else
#  warning "setjmp()/longjmp() is not implemented"
   typedef int pj_jmp_buf[1];
#  define pj_setjmp(buf)	0
#  define pj_longjmp(buf,d)	0
#endif


#endif	/* __PJ_COMPAT_SETJMP_H__ */

