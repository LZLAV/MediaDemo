/**
 * 已完成
 *      文件访问及其访问记录
 */
#ifndef __PJ_FILE_ACCESS_H__
#define __PJ_FILE_ACCESS_H__

/**
 * @file file_access.h
 * @brief 文件操作和访问
 */
#include <pj/types.h>

PJ_BEGIN_DECL 

/**
 * @defgroup PJ_FILE_ACCESS 文件存取
 * @ingroup PJ_IO
 * @{
 *
 */

/**
 * 此结构描述通过调用 pj_file_getstat() 获得的文件信息。此结构中的时间信息是本地时间
 */
typedef struct pj_file_stat
{
    pj_off_t        size;   /**< 总文件大小             */
    pj_time_val     atime;  /**< 最后一次访问时间        */
    pj_time_val     mtime;  /**< 最后一次修改时间        */
    pj_time_val     ctime;  /**< 最后一次创建时间        */
} pj_file_stat;


/**
 * 如果指定的文件存在，则返回非零
 *
 * @param filename      文件名称
 *
 * @return              文件存在返回非零
 */
PJ_DECL(pj_bool_t) pj_file_exists(const char *filename);

/**
 * 返回文件的大小
 *
 * @param filename      文件名称
 *
 * @return              文件大小以字节为单位，错误时为-1
 */
PJ_DECL(pj_off_t) pj_file_size(const char *filename);

/**
 * 删除文件
 *
 * @param filename      文件名称
 *
 * @return              成功返回PJ_SUCCESS,否则返回相应错误码
 */
PJ_DECL(pj_status_t) pj_file_delete(const char *filename);

/**
 * 将oldname 移动到newname。如果 newname 已存在，则将覆盖它
 *
 * @param oldname       要重命名的文件
 * @param newname       要分配的新文件名
 *
 * @return              成功返回PJ_SUCCESS,否则返回相应错误码
 */
PJ_DECL(pj_status_t) pj_file_move( const char *oldname, 
                                   const char *newname);


/**
 * 返回指定文件的有关信息。stat 结构中的时间信息将是本地时间
 *
 * @param filename      文件名称
 * @param stat          指向接收文件信息的变量的指针
 *
 * @return              成功返回PJ_SUCCESS,否则返回相应错误码
 */
PJ_DECL(pj_status_t) pj_file_getstat(const char *filename, pj_file_stat *stat);


/** @} */

PJ_END_DECL


#endif	/* __PJ_FILE_ACCESS_H__ */
