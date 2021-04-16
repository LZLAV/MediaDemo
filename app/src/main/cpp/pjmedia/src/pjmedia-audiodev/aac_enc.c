////
//// Created by lzlbuilder on 2020-03-12.
////
//
//
//#include "aac_enc.h"
//
//
//
//paacenc *getaac_instance() {
//    if (curInstance) {
//        return curInstance;
//    }
//}
//
//paacenc *aac_create(){
//    paacenc *p_aacenc;
//    p_aacenc = malloc(sizeof(paacenc));
//    return p_aacenc;
//}
//
//int aac_open(paacenc *aaenc) {
//
//    AACENC_InfoStruct info ={0};
//
//    aaenc->info =info;
//    aaenc->aot = 2;  //MPEG-4 AAC LC
//
//    aaenc->bitrate = 64000;
//    aaenc->channels = 1;
//    aaenc->sample_rate = 16000;
//    aaenc->vbr = 0;
//    aaenc->afterburner =0;
//
//    aac_file = fopen("/sdcard/test.aac","wb");
//    switch (aaenc->channels) {
//        case 1:
//            aaenc->mode = MODE_1;
//            break;
//        case 2:
//            aaenc->mode = MODE_2;
//            break;
//        case 3:
//            aaenc->mode = MODE_1_2;
//            break;
//        case 4:
//            aaenc->mode = MODE_1_2_1;
//            break;
//        case 5:
//            aaenc->mode = MODE_1_2_2;
//            break;
//        case 6:
//            aaenc->mode = MODE_1_2_2_1;
//            break;
//        default:
//            fprintf(stderr, "Unsupported WAV channels %d\n", aaenc->channels);
//            return 1;
//    }
//    if (aacEncOpen(&aaenc->handle, 0, aaenc->channels) != AACENC_OK) {
//        fprintf(stderr, "Unable to open encoder\n");
//        return 1;
//    }
//    if (aacEncoder_SetParam(aaenc->handle, AACENC_AOT, aaenc->aot) != AACENC_OK) {
//        fprintf(stderr, "Unable to set the AOT\n");
//        return 1;
//    }
//    if (aaenc->aot == 39 && aaenc->eld_sbr) {
//        if (aacEncoder_SetParam(aaenc->handle, AACENC_SBR_MODE, 1) != AACENC_OK) {
//            fprintf(stderr, "Unable to set SBR mode for ELD\n");
//            return 1;
//        }
//    }
//    if (aacEncoder_SetParam(aaenc->handle, AACENC_SAMPLERATE, aaenc->sample_rate) != AACENC_OK) {
//        fprintf(stderr, "Unable to set the AOT\n");
//        return 1;
//    }
//    if (aacEncoder_SetParam(aaenc->handle, AACENC_CHANNELMODE, aaenc->mode) != AACENC_OK) {
//        fprintf(stderr, "Unable to set the channel mode\n");
//        return 1;
//    }
//    if (aacEncoder_SetParam(aaenc->handle, AACENC_CHANNELORDER, 0) != AACENC_OK) {
//        fprintf(stderr, "Unable to set the mpeg channel order\n");
//        return 1;
//    }
//    if (aaenc->vbr) {
//        if (aacEncoder_SetParam(aaenc->handle, AACENC_BITRATEMODE, aaenc->vbr) != AACENC_OK) {
//            fprintf(stderr, "Unable to set the VBR bitrate mode\n");
//            return 1;
//        }
//    } else {
//        if (aacEncoder_SetParam(aaenc->handle, AACENC_BITRATE, aaenc->bitrate) != AACENC_OK) {
//            fprintf(stderr, "Unable to set the bitrate\n");
//            return 1;
//        }
//    }
//    if (aacEncoder_SetParam(aaenc->handle, AACENC_TRANSMUX, TT_MP4_ADTS) != AACENC_OK) {
//        fprintf(stderr, "Unable to set the ADTS transmux\n");
//        return 1;
//    }
//    if (aacEncoder_SetParam(aaenc->handle, AACENC_AFTERBURNER, aaenc->afterburner) != AACENC_OK) {
//        fprintf(stderr, "Unable to set the afterburner mode\n");
//        return 1;
//    }
//    if (aacEncEncode(aaenc->handle, NULL, NULL, NULL, NULL) != AACENC_OK) {
//        fprintf(stderr, "Unable to initialize the encoder\n");
//        return 1;
//    }
//    if (aacEncInfo(aaenc->handle, &aaenc->info) != AACENC_OK) {
//        fprintf(stderr, "Unable to get the encoder info\n");
//        return 1;
//    }
//
//    aaenc->convert_buf = (int16_t *) malloc(2048);
//    curInstance = aaenc;
//
//}
//
//
//int aac_encode(paacenc *aaenc,uint8_t *p_input_buf,int p_input_size,uint8_t *p_out_buf,int* p_out_size) {
//    if (!curInstance) {
//        PJ_LOG(1, (THIS_FILE, "aac encode instance is NULL"));
//        return -1;
//    }
//
//    AACENC_BufDesc in_buf = { 0 }, out_buf = { 0 };
//    AACENC_InArgs in_args = { 0 };
//    AACENC_OutArgs out_args = { 0 };
//    int in_identifier = IN_AUDIO_DATA;
//    int in_size, in_elem_size;
//    int out_identifier = OUT_BITSTREAM_DATA;
//    int out_size, out_elem_size;
//    int read, i;
//    void *in_ptr, *out_ptr;
//    AACENC_ERROR err;
//
//    read = p_input_size;
//    for (i = 0; i < read/2; i++) {
//        const uint8_t* in = &p_input_buf[2*i];
//        aaenc->convert_buf[i] = in[0] | (in[1] << 8);
//    }
//    in_ptr = aaenc->convert_buf;
//    in_size = read;
//    in_elem_size = 2;
//
//    in_args.numInSamples = read <= 0 ? -1 : read/2;
//    in_buf.numBufs = 1;
//    in_buf.bufs = &in_ptr;
//    in_buf.bufferIdentifiers = &in_identifier;
//    in_buf.bufSizes = &in_size;
//    in_buf.bufElSizes = &in_elem_size;
//
//    out_ptr = p_out_buf;
//    out_size = 2048;
//    out_elem_size = 1;
//    out_buf.numBufs = 1;
//    out_buf.bufs = &out_ptr;
//    out_buf.bufferIdentifiers = &out_identifier;
//    out_buf.bufSizes = &out_size;
//    out_buf.bufElSizes = &out_elem_size;
//
//    if ((err = aacEncEncode(aaenc->handle, &in_buf, &out_buf, &in_args, &out_args)) != AACENC_OK) {
//        if (err == AACENC_ENCODE_EOF)
//            fprintf(stderr, "Encoding failed\n");
//        return -1;
//    }
//    if (out_args.numOutBytes == 0)
//        return -1;
//    *p_out_size = out_args.numOutBytes;
////    fwrite(p_out_buf,1, out_args.numOutBytes,aac_file);
//    return 0;
//}
//
//void aac_destroy() {
//    if(aac_file) {
//        fclose(aac_file);
//    }
//    if(curInstance && curInstance->convert_buf) {
//        free(curInstance->convert_buf);
//    }
//    if(curInstance && &curInstance->handle) {
//        aacEncClose(&curInstance->handle);
//    }
//    if(curInstance) {
//        free(curInstance);
//    }
//}