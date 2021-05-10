/**
 * 已完成
 *      文件操作：
 *          打开
 *          关闭
 *          读取
 *          写入
 *          获取当前位置
 *          设置位置
 */
#ifndef __PJ_FILE_IO_H__
#define __PJ_FILE_IO_H__

/**
 * @file file_io.h
 * @brief Simple 文件 I/O 抽象
 */
#include <pj/types.h>

PJ_BEGIN_DECL 

/**
 * @defgroup PJ_FILE_IO 文件I/O
 * @ingroup PJ_IO
 * @{
 *
 * 此文件包含执行文件I/O的功能。文件I/O可以通过各种后端实现，可以使用本机文件 API 或 ANSI 流
 *
 * @section pj_file_size_limit_sec 大小限制
 *
 * pj_file_setpos() 或 pj_file_getpos() 函数可以处理的大小可能有限制。API本身使用64位整数作为文件偏移量/位置（如果可用）；
 * 但是，某些后端（如 ANSI）可能只支持有符号32位偏移分辨率
 *
 * 读写操作使用有符号的32位整数来表示大小
 *
 */

/**
 * 打开文件时使用这些枚举。值PJ_O_RDONLY、PJ_O_WRONLY和PJ_O_RDWR是互斥的。值PJ_O_APPEND只能在打开文件进行写入时使用
 */
enum pj_file_access
{
    PJ_O_RDONLY     = 0x1101,   /**< 读打开文件      */
    PJ_O_WRONLY     = 0x1102,   /**< 写打开文件      */
    PJ_O_RDWR       = 0x1103,   /**< 读写打开文件，文件将被截断     */
    PJ_O_APPEND     = 0x1108    /**< 附加到已存在的文件        */
};

/**
 * 使用pj_file_setpos设置文件位置时的seek指令
 */
enum pj_file_seek_type
{
    PJ_SEEK_SET     = 0x1201,   /**< 文件起始   */
    PJ_SEEK_CUR     = 0x1202,   /**< 文件当前位置     */
    PJ_SEEK_END     = 0x1203    /**< 文件末尾     */
};

/**
 * 以指定的模式打开指定路径的文件，并在 fd中返回句柄。所有文件都将以二进制格式打开
 *
 * @param pool          池为新文件描述符分配内存
 * @param pathname      打开文件的名称
 * @param flags         打开文件标识 flags，是 pj_file_access 的位掩码组合。标志必须是PJ_O_RDONLY、PJ_O_WRONLY或P
 *                      PJ_O_RDWR。指定文件写入时，除非指定PJ_O_APPEND，否则现有文件将被截断。
 * @param fd            返回的文件描述符
 *
 * @return              成功返回 PJ_SUCCESS，否则返回相应的错误码
 */
PJ_DECL(pj_status_t) pj_file_open(pj_pool_t *pool,
                                  const char *pathname, 
                                  unsigned flags,
                                  pj_oshandle_t *fd);

/**
 * 关闭一个打开的文件描述符
 *
 * @param fd            文件描述符
 *
 * @return              成功返回 PJ_SUCCESS，否则返回相应的错误码
 */
PJ_DECL(pj_status_t) pj_file_close(pj_oshandle_t fd);

/**
 * 将指定大小的数据写入打开的文件
 *
 * @param fd            文件描述符
 * @param data          要写入文件的数据
 * @param size          输入时，指定要写入的数据的大小。
 *                      返回时，它包含实际写入文件的数据数
 *
 * @return              成功返回 PJ_SUCCESS，否则返回相应的错误码
 */
PJ_DECL(pj_status_t) pj_file_write(pj_oshandle_t fd,
                                   const void *data,
                                   pj_ssize_t *size);

/**
 * 从指定文件读取数据。当设置文件结束条件时，此函数将返回 PJ_SUCCESS，但大小将包含零
 *
 * @param fd            文件描述符
 * @param data          指向接收数据的缓冲区的指针
 * @param size          输入时，指定从文件中读取的最大数据数。在输出时，它包含实际从文件中读取的数据的大小。当 EOF发生时，它将包含零。
 *
 * @return              成功返回 PJ_SUCCESS，否则返回相应的错误码
 *                      当EOF发生时，返回的是PJ_SUCCESS，但size将报告为零
 */
PJ_DECL(pj_status_t) pj_file_read(pj_oshandle_t fd,
                                  void *data,
                                  pj_ssize_t *size);

/**
 * 根据指令 whence 将文件位置设置为新偏移量
 *
 * @param fd            文件描述符
 * @param offset        要设置的新文件位置
 * @param whence        参考位置
 *
 * @return              成功返回 PJ_SUCCESS，否则返回相应的错误码
 */
PJ_DECL(pj_status_t) pj_file_setpos(pj_oshandle_t fd,
                                    pj_off_t offset,
                                    enum pj_file_seek_type whence);

/**
 * 获取当前文件的位置
 *
 * @param fd            文件描述符
 * @param pos           返回时包含从文件开头开始测量的文件位置
 *
 * @return              成功返回 PJ_SUCCESS，否则返回相应的错误码
 */
PJ_DECL(pj_status_t) pj_file_getpos(pj_oshandle_t fd,
                                    pj_off_t *pos);

/**
 * 刷新文件缓存区
 *
 * @param fd		文件描述符
 *
 * @return		成功返回 PJ_SUCCESS，否则返回相应的错误码
 */
PJ_DECL(pj_status_t) pj_file_flush(pj_oshandle_t fd);


/** @} */


PJ_END_DECL

#endif  /* __PJ_FILE_IO_H__ */

