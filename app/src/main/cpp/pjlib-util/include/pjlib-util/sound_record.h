//
// Created by SH Qiu on 2020-06-30.
//

#ifndef __PJLIB_UTIL_SOUND_RECORD_H
#define __PJLIB_UTIL_SOUND_RECORD_H

#include <pjlib-util/types.h>
#include <pj/types.h>
#include <pjmedia/frame.h>

PJ_BEGIN_DECL

PJ_DECL(void)pj_sound_record_init(void);


PJ_DECL(void)pj_adjust_tx_level(int slot, float level);


PJ_DECL(void)pj_adjust_rx_level(int slot, float level);

/**
 * @brief 声音采集后记录(不做任何处理)
 */
PJ_DECL(void)pj_sound_mic_record(pjmedia_frame *frame);

/**
 * @brief 声音采集经过回声抑制后记录
 */
PJ_DECL(void)pj_sound_mic_record_echo_after(pjmedia_frame *frame);

/**
 * @brief 声音采集发送之前(编码后)
 */
PJ_DECL(void)pj_sound_mic_record_send_before(pjmedia_frame *frame);

/**
 * @brief 声音接收记录(解码之前)
 */
PJ_DECL(void)pj_sound_receive_record_decodec_before(pjmedia_frame *frame, int samples_per_frame);


/**
 * @brief 声音接收记录(播放之前)
 */
PJ_DECL(void)pj_sound_receive_record_play_before(pjmedia_frame *frame, int samples_per_frame);


PJ_DECL(void)pj_sound_record_deinit(void);


PJ_END_DECL

#endif /* __PJLIB_UTIL_SOUND_RECORD_H */
