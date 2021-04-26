/* $Id: circbuf.h 4537 2013-06-19 06:47:43Z riza $ */
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

#ifndef __PJMEDIA_CIRC_BUF_H__
#define __PJMEDIA_CIRC_BUF_H__

/**
 * @file circbuf.h
 * @brief 循环buf
 */

#include <pj/assert.h>
#include <pj/errno.h>
#include <pj/pool.h>
#include <pjmedia/frame.h>

/**
 * @defgroup PJMED_CIRCBUF 循环buf
 * @ingroup PJMEDIA_FRAME_OP
 * @brief	循环缓冲区在非连续缓冲区中管理读写连续音频样本，就好像缓冲区是连续的一样。这应该比在连续的缓冲区中保持连
 * 			续的样本提供更好的性能，因为读/写操作只会更新指针，而不是移动音频样本
 *
 * @{
 *
 * 本节描述 PJMEDIA 的循环缓冲区实现
 *
 */

/* 算法检查，仅供开发使用  */
#if 0
#   define PJMEDIA_CIRC_BUF_CHECK(x) pj_assert(x)
#else
#   define PJMEDIA_CIRC_BUF_CHECK(x)
#endif

PJ_BEGIN_DECL

/** 
 * 循环buf 结构
 */
typedef struct pjmedia_circ_buf {
    pj_int16_t *buf;        /**< 缓存		*/
    unsigned capacity;        /**< 缓存容量,单位帧采样	*/

    pj_int16_t *start;        /**< 首帧采样地址	*/
    unsigned len;        /**< 音频样本长度，以样本为单位 */
} pjmedia_circ_buf;


/**
 * 创建一个循环buf
 *
 * @param pool		    循环缓冲区将从中分配的池。
 * @param capacity	    buf 容量, 以样本为单位
 * @param p_cb		    接收循环缓冲区实例的指针
 *
 * @return		    如果已成功创建循环缓冲区，则返回 PJ_SUCCESS，否则返回相应的错误
 */
PJ_INLINE(pj_status_t) pjmedia_circ_buf_create(pj_pool_t *pool,
                                               unsigned capacity,
                                               pjmedia_circ_buf **p_cb) {
    pjmedia_circ_buf *cbuf;

    cbuf = PJ_POOL_ZALLOC_T(pool, pjmedia_circ_buf);
    cbuf->buf = (pj_int16_t *) pj_pool_calloc(pool, capacity,
                                              sizeof(pj_int16_t));
    cbuf->capacity = capacity;
    cbuf->start = cbuf->buf;
    cbuf->len = 0;

    *p_cb = cbuf;

    return PJ_SUCCESS;
}


/**
 * 重置循环buf
 *
 * @param circbuf	    循环buf
 *
 * @return		    成功返回 PJ_SUCCESS
 */
PJ_INLINE(pj_status_t) pjmedia_circ_buf_reset(pjmedia_circ_buf *circbuf) {
    circbuf->start = circbuf->buf;
    circbuf->len = 0;

    return PJ_SUCCESS;
}


/**
 * 获取循环缓冲区长度，它是循环缓冲区中缓冲的样本数。
 *
 * @param circbuf	   循环缓冲区
 *
 * @return		    缓存区的长度
 */
PJ_INLINE(unsigned) pjmedia_circ_buf_get_len(pjmedia_circ_buf *circbuf) {
    return circbuf->len;
}


/**
 * 设置循环缓冲区长度。这在用户手动操作音频缓冲区时非常有用，例如：缩小、扩展
 *
 * @param circbuf	    循环缓存区
 * @param len		    新的buf 长度
 */
PJ_INLINE(void) pjmedia_circ_buf_set_len(pjmedia_circ_buf *circbuf,
                                         unsigned len) {
    PJMEDIA_CIRC_BUF_CHECK(len <= circbuf->capacity);
    circbuf->len = len;
}


/**
 * 前进循环缓冲区的读取指针。此函数将在前进读取指针时丢弃跳过的样本，从而减少缓冲区长度
 *
 * @param circbuf	    The circular buffer.
 * @param count		    距离当前读取指针的距离，只能是正数，在样本中
 *
 * @return		    成功时返回 PJ_SUCCESS，否则返回相应的错误码
 */
PJ_INLINE(pj_status_t) pjmedia_circ_buf_adv_read_ptr(pjmedia_circ_buf *circbuf,
                                                     unsigned count) {
    if (count >= circbuf->len)
        return pjmedia_circ_buf_reset(circbuf);

    PJMEDIA_CIRC_BUF_CHECK(count <= circbuf->len);

    circbuf->start += count;
    if (circbuf->start >= circbuf->buf + circbuf->capacity)
        circbuf->start -= circbuf->capacity;
    circbuf->len -= count;

    return PJ_SUCCESS;
}


/**
 * 推进循环缓冲区的写入指针。因为write指针总是指向 sample 结束后的一个 sample，所以这个函数还意味着增加缓冲区长度。
 *
 * @param circbuf	    循环buf
 * @param count		    距离当前写指针的距离，只能是正数，在样本中
 *
 * @return		    成功返回 PJ_SUCCESS，否则返回相应的错误码
 */
PJ_INLINE(pj_status_t) pjmedia_circ_buf_adv_write_ptr(pjmedia_circ_buf *circbuf,
                                                      unsigned count) {
    if (count + circbuf->len > circbuf->capacity)
        return PJ_ETOOBIG;

    circbuf->len += count;

    return PJ_SUCCESS;
}


/**
 * 获取包含音频样本的真实缓冲区地址
 *
 * @param circbuf	    循环buf
 * @param reg1		    存储第一个缓冲区地址的指针
 * @param reg1_len	    用于存储第一个缓冲区长度的指针，以样本为单位
 * @param reg2		    存储第二个缓冲区地址的指针
 * @param reg2_len	    用于存储第二个缓冲区长度的指针，以样本为单位
 */
PJ_INLINE(void) pjmedia_circ_buf_get_read_regions(pjmedia_circ_buf *circbuf,
                                                  pj_int16_t **reg1,
                                                  unsigned *reg1_len,
                                                  pj_int16_t **reg2,
                                                  unsigned *reg2_len) {
    *reg1 = circbuf->start;
    *reg1_len = circbuf->len;
    if (*reg1 + *reg1_len > circbuf->buf + circbuf->capacity) {
        *reg1_len = (unsigned) (circbuf->buf + circbuf->capacity -
                                circbuf->start);
        *reg2 = circbuf->buf;
        *reg2_len = circbuf->len - *reg1_len;
    } else {
        *reg2 = NULL;
        *reg2_len = 0;
    }

    PJMEDIA_CIRC_BUF_CHECK(*reg1_len != 0 || (*reg1_len == 0 &&
                                              circbuf->len == 0));
    PJMEDIA_CIRC_BUF_CHECK(*reg1_len + *reg2_len == circbuf->len);
}


/**
 * 获取空的或可写的实际缓冲区地址
 *
 * @param circbuf	    循环buf
 * @param reg1		    存储第一个缓冲区地址的指针
 * @param reg1_len	    用于存储第一个缓冲区长度的指针，以样本为单位
 * @param reg2		    存储第二个缓冲区地址的指针
 * @param reg2_len	    用于存储第二个缓冲区长度的指针，以样本为单位
 */
PJ_INLINE(void) pjmedia_circ_buf_get_write_regions(pjmedia_circ_buf *circbuf,
                                                   pj_int16_t **reg1,
                                                   unsigned *reg1_len,
                                                   pj_int16_t **reg2,
                                                   unsigned *reg2_len) {
    *reg1 = circbuf->start + circbuf->len;
    if (*reg1 >= circbuf->buf + circbuf->capacity)
        *reg1 -= circbuf->capacity;
    *reg1_len = circbuf->capacity - circbuf->len;
    if (*reg1 + *reg1_len > circbuf->buf + circbuf->capacity) {
        *reg1_len = (unsigned) (circbuf->buf + circbuf->capacity - *reg1);
        *reg2 = circbuf->buf;
        *reg2_len = (unsigned) (circbuf->start - circbuf->buf);
    } else {
        *reg2 = NULL;
        *reg2_len = 0;
    }

    PJMEDIA_CIRC_BUF_CHECK(*reg1_len != 0 || (*reg1_len == 0 &&
                                              circbuf->len == 0));
    PJMEDIA_CIRC_BUF_CHECK(*reg1_len + *reg2_len == circbuf->capacity -
                                                    circbuf->len);
}


/**
 * 从循环缓冲区读取音频样本
 *
 * @param circbuf	    循环buf
 * @param data		    缓冲区来存储读取的音频样本
 * @param count		    正在读取的样本数
 *
 * @return		    成功返回PJ_SUCCESS，否则返回相应的错误码
 */
PJ_INLINE(pj_status_t) pjmedia_circ_buf_read(pjmedia_circ_buf *circbuf,
                                             pj_int16_t *data,
                                             unsigned count) {
    pj_int16_t *reg1, *reg2;
    unsigned reg1cnt, reg2cnt;

    /* 缓冲区中的数据少于请求的数据 */
    if (count > circbuf->len)
        return PJ_ETOOBIG;

    pjmedia_circ_buf_get_read_regions(circbuf, &reg1, &reg1cnt,
                                      &reg2, &reg2cnt);
    if (reg1cnt >= count) {
        pjmedia_copy_samples(data, reg1, count);
    } else {
        pjmedia_copy_samples(data, reg1, reg1cnt);
        pjmedia_copy_samples(data + reg1cnt, reg2, count - reg1cnt);
    }

    return pjmedia_circ_buf_adv_read_ptr(circbuf, count);
}


/**
 * 将音频样本写入循环缓冲区。
 *
 * @param circbuf	    循环buf
 * @param data		    要编写的音频样本
 * @param count		    正在写入的样本数
 *
 * @return		    成功返回 PJ_SUCCESS，否则返回相应的错误码
 */
PJ_INLINE(pj_status_t) pjmedia_circ_buf_write(pjmedia_circ_buf *circbuf,
                                              pj_int16_t *data,
                                              unsigned count) {
    pj_int16_t *reg1, *reg2;
    unsigned reg1cnt, reg2cnt;

    /* 要写入的数据大于缓冲区所存储的数据 */
    if (count > circbuf->capacity - circbuf->len)
        return PJ_ETOOBIG;

    pjmedia_circ_buf_get_write_regions(circbuf, &reg1, &reg1cnt,
                                       &reg2, &reg2cnt);
    if (reg1cnt >= count) {
        pjmedia_copy_samples(reg1, data, count);
    } else {
        pjmedia_copy_samples(reg1, data, reg1cnt);
        pjmedia_copy_samples(reg2, data + reg1cnt, count - reg1cnt);
    }

    return pjmedia_circ_buf_adv_write_ptr(circbuf, count);
}


/**
 * 从循环缓冲区复制音频样本，而不更改其状态
 *
 * @param circbuf	    循环buf
 * @param start_idx	    开始复制样本索引
 * @param data		    缓冲区来存储读取的音频样本
 * @param count		    正在读取的样本数
 *
 * @return		    成功返回PJ_SUCCESS，否则返回相应的错误码
 */
PJ_INLINE(pj_status_t) pjmedia_circ_buf_copy(pjmedia_circ_buf *circbuf,
                                             unsigned start_idx,
                                             pj_int16_t *data,
                                             unsigned count) {
    pj_int16_t *reg1, *reg2;
    unsigned reg1cnt, reg2cnt;

    /* 缓冲区中的数据少于请求的数据 */
    if (count + start_idx > circbuf->len)
        return PJ_ETOOBIG;

    pjmedia_circ_buf_get_read_regions(circbuf, &reg1, &reg1cnt,
                                      &reg2, &reg2cnt);
    if (reg1cnt > start_idx) {
        unsigned tmp_len;
        tmp_len = reg1cnt - start_idx;
        if (tmp_len > count)
            tmp_len = count;
        pjmedia_copy_samples(data, reg1 + start_idx, tmp_len);
        if (tmp_len < count)
            pjmedia_copy_samples(data + tmp_len, reg2, count - tmp_len);
    } else {
        pjmedia_copy_samples(data, reg2 + start_idx - reg1cnt, count);
    }

    return PJ_SUCCESS;
}


/**
 * 打包缓冲区，使第一个样本位于缓冲区的开头
 * 这也将使缓冲区连续
 *
 * @param circbuf	    循环buf
 *
 * @return		    成功返回 PJ_SUCCESS,否则返回相应错误码
 */
PJ_INLINE(pj_status_t) pjmedia_circ_buf_pack_buffer(pjmedia_circ_buf *circbuf) {
    pj_int16_t *reg1, *reg2;
    unsigned reg1cnt, reg2cnt;
    unsigned gap;

    pjmedia_circ_buf_get_read_regions(circbuf, &reg1, &reg1cnt,
                                      &reg2, &reg2cnt);

    /* 不连续检查 */
    if (reg2cnt != 0) {
        /*
         * 检查是否没有空间来滚动缓冲区（或者该功能是否提供临时缓冲区？）
         */
        gap = circbuf->capacity - pjmedia_circ_buf_get_len(circbuf);
        if (gap == 0)
            return PJ_ETOOBIG;

        /* 使用间隙向左滚动缓冲区，直到 reg2cnt==0 */
        do {
            if (gap > reg2cnt)
                gap = reg2cnt;
            pjmedia_move_samples(reg1 - gap, reg1, reg1cnt);
            pjmedia_copy_samples(reg1 + reg1cnt - gap, reg2, gap);
            if (gap < reg2cnt)
                pjmedia_move_samples(reg2, reg2 + gap, reg2cnt - gap);
            reg1 -= gap;
            reg1cnt += gap;
            reg2cnt -= gap;
        } while (reg2cnt > 0);
    }

    /* 最后，将采样移到左边缘 */
    if (reg1 != circbuf->buf)
        pjmedia_move_samples(circbuf->buf, reg1,
                             pjmedia_circ_buf_get_len(circbuf));
    circbuf->start = circbuf->buf;

    return PJ_SUCCESS;
}


PJ_END_DECL

/**
 * @}
 */

#endif
