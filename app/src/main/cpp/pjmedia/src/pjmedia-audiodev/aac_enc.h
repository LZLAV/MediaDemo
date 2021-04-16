///*
////
//// Created by lzlbuilder on 2020-03-15.
////
//
//#ifndef CSIPSDK_NDK_AAC_ENC_H
//#define CSIPSDK_NDK_AAC_ENC_H
//
//#include <stdio.h>
//#include <stdint.h>
//
//#if defined(_MSC_VER)
//#include <getopt.h>
//#else
//
//#include <unistd.h>
//
//#endif
//
//#include <stdlib.h>
//#include <FDK_audio.h>
//#include <pj/log.h>
//#include "aacenc_lib.h"
//
//#define THIS_FILE "aac_enc.c"
//
//typedef struct paacenc {
//    int bitrate;
//    int format;
//    int channels;
//    int sample_rate;
//    int bits_per_sample;
//    int aot;
//    int afterburner;
//    int eld_sbr;
//    int vbr;
//
//    int16_t *convert_buf;
//    HANDLE_AACENCODER handle;
//    CHANNEL_MODE mode;
//    AACENC_InfoStruct info;
//} paacenc;
//
//paacenc *curInstance;
//
//FILE *aac_file;
//
//paacenc *getaac_instance();
//
//paacenc *aac_create();
//
//int aac_open(paacenc *aaenc);
//
//int aac_encode(paacenc *aaenc,uint8_t *p_input_buf,int p_input_size,uint8_t *p_out_buf,int* p_out_size);
//
//void aac_destroy();
//
//
//
//#endif //CSIPSDK_NDK_AAC_ENC_H
//*/
