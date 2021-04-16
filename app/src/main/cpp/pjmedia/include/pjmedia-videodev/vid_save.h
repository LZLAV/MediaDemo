//
// Created by lzlbuilder on 2020-03-03.
//

#ifndef CSIPSDK_VID_SAVE_H
#define CSIPSDK_VID_SAVE_H

#include <pj/log.h>
#include <jni.h>
#include <pjmedia-videodev/errno.h>
#include <pjmedia/frame.h>

#define SDK_UP_19 2
#define SDK_BELOW_19 1

static pj_status_t jni_init_ids();

static char* jstringTostring(JNIEnv* env, jstring jstr);

int writePng(unsigned char*  pixels , int width, int height, int bit_depth);

pj_bool_t get_take_pic_running_state();

void set_take_pic_state(pj_bool_t b);

pj_bool_t get_vid_record_state();

pj_status_t vid_save_set_aud_param(unsigned int bitRate,unsigned int sampleRate,unsigned int channel,
                                   unsigned int sample_bit,unsigned int format);

pj_status_t vid_save_set_vid_param(unsigned int width,unsigned int height,
                                   unsigned int bitRate,unsigned int fps);

pj_status_t vid_save_add_vid_data(void *buf,int size);
pj_status_t vid_save_add_aud_data(void *buf,int size);

pj_bool_t vid_save_register();

#endif //CSIPSDK_VID_SAVE_H
