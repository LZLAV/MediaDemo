/**
 * 已完成
 *      C 类型的字符
 */
#ifndef __PJ_CTYPE_H__
#define __PJ_CTYPE_H__

/**
 * @file ctype.h
 * @brief C类型帮助宏
 */

#include <pj/types.h>
#include <pj/compat/ctype.h>

PJ_BEGIN_DECL

/**
 * @defgroup pj_ctype ctype - 字符类型
 * @ingroup PJ_MISC
 * @{
 *
 * 此模块包含多个内联函数/宏，用于测试或操作字符类型。它在 PJLIB 中提供，因为 PJLIB不能依赖于LIBC
 */

/**
 * 如果是一个C 格式的字符或是数字，则返回非零值
 * @param c     要测试的整数字符
 * @return      如果是一个字符或是数字，则返回非零值
 */
PJ_INLINE(int) pj_isalnum(unsigned char c) { return isalnum(c); }

/** 
 * 如果c是字母字符的特定表示形式，则返回非零值
 * @param c     要测试的整数字符
 * @return      如果c是字母字符的特殊表示，则为非零值
 */
PJ_INLINE(int) pj_isalpha(unsigned char c) { return isalpha(c); }

/** 
 * 如果 c 是ASCII字符的特定表示，则返回非零值
 * @param c     要测试的整数字符
 * @return      如果 c 是ASCII字符的特定表示，则返回非零值
 */
PJ_INLINE(int) pj_isascii(unsigned char c) { return c<128; }

/** 
 * 如果c是十进制数字字符的特定表示形式，则返回非零值
 * @param c     要测试的整数字符
 * @return      如果c是十进制数字字符的特定表示形式，则返回非零值
 */
PJ_INLINE(int) pj_isdigit(unsigned char c) { return isdigit(c); }

/**
 * 如果 c 是空格字符的特定表示形式（0x09-0x0D或0x20），则返回非零值
 * @param c     要测试的整数字符
 * @return      如果 c 是空格字符的特定表示形式（0x09-0x0D或0x20），则返回非零值
 */
PJ_INLINE(int) pj_isspace(unsigned char c) { return isspace(c); }

/** 
 * 如果c是小写字符的特定表示形式，则返回非零值
 * @param c     要测试的整数字符
 * @return      如果c是小写字符的特定表示形式，则返回非零值
 */
PJ_INLINE(int) pj_islower(unsigned char c) { return islower(c); }


/** 
 * 如果c是大写字符的特定表示形式，则返回非零值。
 * @param c     要测试的整数字符
 * @return      如果c是大写字符的特定表示形式，则返回非零值
 */
PJ_INLINE(int) pj_isupper(unsigned char c) { return isupper(c); }

/**
 * 检测是否为空格或是水平制表符
 * @param c     要测试的整数字符
 * @return      是空格或水平制表符则返回非零值
 */
PJ_INLINE(int) pj_isblank(unsigned char c) { return (c==' ' || c=='\t'); }

/**
 * 转换字符为小写
 * @param c     要转换的整数字符
 * @return      转换后的小写字符
 */
PJ_INLINE(int) pj_tolower(unsigned char c) { return tolower(c); }

/**
 * 转换字符为大写字符
 * @param c     要转换的整数字符
 * @return      转换后的大写字符
 */
PJ_INLINE(int) pj_toupper(unsigned char c) { return toupper(c); }

/**
 * 检测是否为十六进制数字字符
 * @param c     要检测的整数字符
 * @return      是十六进制字符则返回非零值
 */
PJ_INLINE(int) pj_isxdigit(unsigned char c){ return isxdigit(c); }

/**
 * 十六进制字符数组，小写
 */
/*extern char pj_hex_digits[];*/
#define pj_hex_digits	"0123456789abcdef"

/**
 * 转换十进制为十六进制
 * @param value	    要转换的值
 * @param p	    缓冲区来保存十六进制表示，它必须至少有两个字节的长度
 */
PJ_INLINE(void) pj_val_to_hex_digit(unsigned value, char *p)
{
    *p++ = pj_hex_digits[ (value & 0xF0) >> 4 ];
    *p   = pj_hex_digits[ (value & 0x0F) ];
}

/**
 * 转换十六进制为十进制
 * @param c	要转换的十六进制
 * @return	转换后的值，0~15
 */
PJ_INLINE(unsigned) pj_hex_digit_to_val(unsigned char c)
{
    if (c <= '9')
	return (c-'0') & 0x0F;
    else if (c <= 'F')
	return  (c-'A'+10) & 0x0F;
    else
	return (c-'a'+10) & 0x0F;
}

/** @} */

PJ_END_DECL

#endif	/* __PJ_CTYPE_H__ */

