//
// Created by lzlbuilder on 2020-03-03.
//

#include <pjmedia-videodev/vid_save.h>
#include <third_party/libpng/png.h>
#include <stdbool.h>


#if defined(PJMEDIA_HAS_VIDEO) && PJMEDIA_HAS_VIDEO != 0 && \
    defined(PJMEDIA_VIDEO_DEV_HAS_ANDROID) && \
    PJMEDIA_VIDEO_DEV_HAS_ANDROID != 0

#define THIS_FILE        "vid_save.c"

static pj_bool_t video_record_running = PJ_FALSE;
static pj_bool_t is_take_pic = PJ_FALSE;

extern JavaVM *pj_jni_jvm;
jobject jvid_recoder;
jbyte *vid_buf;
jbyteArray vid_output;

pj_bool_t quit;

jbyte *aud_buf;
jbyteArray aud_output;

#define VIDEO_RECODER_CLASS_PATH "org/pjsip/recorder/VideoRecorder"



struct jni_objs_t{
    jclass cls;
    jmethodID m_add_aud_data;
    jmethodID m_add_vid_data;
    jmethodID m_start;
    jmethodID m_stop;
} vid_recoder;

static char* jstringTostring(JNIEnv* env, jstring jstr)
{
    char* rtn = NULL;
    jclass clsstring = (*env)->FindClass(env,"java/lang/String");
    jstring strencode = (*env)->NewStringUTF(env,"utf-8");
    jmethodID mid = (*env)->GetMethodID(env,clsstring, "getBytes", "(Ljava/lang/String;)[B");
    jbyteArray barr= (jbyteArray)(*env)->CallObjectMethod(env,jstr, mid, strencode);
    jsize alen = (*env)->GetArrayLength(env,barr);
    jbyte* ba = (*env)->GetByteArrayElements(env,barr, JNI_FALSE);
    if (alen > 0)
    {
        rtn = (char*)malloc(alen + 1);
        memcpy(rtn, ba, alen);
        rtn[alen] = '\0';
    }
    (*env)->ReleaseByteArrayElements(env,barr, ba, 0);
    return rtn;
}


static pj_bool_t jni_get_env(JNIEnv **jni_env) {
    pj_bool_t with_attach = PJ_FALSE;
    if ((*pj_jni_jvm)->GetEnv(pj_jni_jvm, (void **) jni_env,
                              JNI_VERSION_1_4) < 0) {
        if ((*pj_jni_jvm)->AttachCurrentThread(pj_jni_jvm, jni_env, NULL) < 0) {
            *jni_env = NULL;
        } else {
            with_attach = PJ_TRUE;
        }
    }
    return with_attach;
}

static void jni_detach_env() {
    JNIEnv *jni_env;
    if (jni_get_env(&jni_env))
        (*pj_jni_jvm)->DetachCurrentThread(pj_jni_jvm);
}

static void init(JNIEnv *env,jobject obj){

    //获取实例对象
    jvid_recoder = (jobject) (*env)->NewGlobalRef(env, obj);
    if (jvid_recoder == NULL) {
        PJ_LOG(3, (THIS_FILE, "Unable to create global ref to VideoRedorder"));
        return;
    }
    quit = PJ_FALSE;
    //初始化
    jni_init_ids();

}

char* pic_file_name=NULL;

int writePng( unsigned char*  pixels , int width, int height, int bit_depth)
{
    png_structp png_ptr;
    png_infop info_ptr;

    if(!pic_file_name){
        PJ_LOG(1,(THIS_FILE,"pic file name empty"));
        return -1;
    }
    FILE *png_file = fopen(pic_file_name, "wb");
    if (!png_file)
    {
        return -1;
    }
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if(png_ptr == NULL)
    {
        PJ_LOG(3,(THIS_FILE,"ERROR:png_create_write_struct/n"));
        fclose(png_file);
        return 0;
    }
    info_ptr = png_create_info_struct(png_ptr);
    if(info_ptr == NULL)
    {
        PJ_LOG(3,(THIS_FILE,"ERROR:png_create_info_struct/n"));
        png_destroy_write_struct(&png_ptr, NULL);
        return 0;
    }
    png_init_io(png_ptr, png_file);
    png_set_IHDR(png_ptr, info_ptr, width, height, bit_depth, /*PNG_COLOR_TYPE_PALETTE*/PNG_COLOR_TYPE_RGB_ALPHA,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);


    png_colorp palette = (png_colorp)png_malloc(png_ptr, PNG_MAX_PALETTE_LENGTH * sizeof(png_color));
    if (!palette) {
        fclose(png_file);
        png_destroy_write_struct(&png_ptr, &info_ptr);
        return false;
    }
    png_set_PLTE(png_ptr, info_ptr, palette, PNG_MAX_PALETTE_LENGTH);
    png_write_info(png_ptr, info_ptr);
    png_set_packing(png_ptr);
    //这里就是图像数据了  BGRA --->RGBA
    int pos =0;
    int channel = 4;
    png_bytepp rows = (png_bytepp)png_malloc(png_ptr, height * sizeof(png_bytep));
    for (int i = 0; i < height; ++i)
    {
        rows[i] = malloc(channel*width* sizeof(unsigned char));
        for(int j=0;j<channel*width;){
            rows[i][j++] = pixels[pos+2];
            rows[i][j++] = pixels[pos+1];
            rows[i][j++] = pixels[pos];
            rows[i][j++] = pixels[pos+3];
            pos+=channel;
        }
    }

    png_write_image(png_ptr, rows);
    png_write_end(png_ptr, info_ptr);
    for (int i = 0; i <height ; ++i) {
        free(rows[i]);
    }
    png_free(png_ptr,rows);
    rows =NULL;
    png_free(png_ptr, palette);
    palette=NULL;
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(png_file);
    return 0;
}

static void JNICALL take_pic(JNIEnv *env, jobject obj,jstring pic_path) {
    pic_file_name = jstringTostring(env,pic_path);
    is_take_pic = PJ_TRUE;
}


static void JNICALL start(JNIEnv *env, jobject obj,jint type) {
    video_record_running = type;
    (*env)->CallVoidMethod(env, obj,vid_recoder.m_start);

}

static void JNICALL stop(JNIEnv *env, jobject obj) {
    video_record_running = PJ_FALSE;
    (*env)->CallVoidMethod(env, obj,
                          vid_recoder.m_stop);

}


static pj_status_t jni_init_ids() {
    JNIEnv *jni_env;
    pj_status_t status = PJ_SUCCESS;
    pj_bool_t with_attach = jni_get_env(&jni_env);

#define GET_METHOD_ID(cls, class_name, method_name, signature, id) \
    id = (*jni_env)->GetMethodID(jni_env, cls, method_name, signature); \
    if (id == 0) { \
        PJ_LOG(3, (THIS_FILE, "[JNI] Unable to find method '" method_name \
                  "' in class '" class_name "'")); \
        status = PJMEDIA_EVID_SYSERR; \
        goto on_return; \
    }
#define GET_SMETHOD_ID(cls, class_name, method_name, signature, id) \
    id = (*jni_env)->GetStaticMethodID(jni_env, cls, method_name, signature); \
    if (id == 0) { \
        PJ_LOG(3, (THIS_FILE, "[JNI] Unable to find static method '" \
                  method_name "' in class '" class_name "'")); \
        status = PJMEDIA_EVID_SYSERR; \
        goto on_return; \
    }
#define GET_FIELD_ID(cls, class_name, field_name, signature, id) \
    id = (*jni_env)->GetFieldID(jni_env, cls, field_name, signature); \
    if (id == 0) { \
        PJ_LOG(3, (THIS_FILE, "[JNI] Unable to find field '" field_name \
                  "' in class '" class_name "'")); \
        status = PJMEDIA_EVID_SYSERR; \
        goto on_return; \
    }


//    GET_METHOD_ID(vid_recoder.cls, "VideoRecorder", "setAudParams", "(IIIII)V",
//                  vid_recoder.m_set_aud_param);
//    GET_METHOD_ID(vid_recoder.cls, "VideoRecorder", "setVidParams", "(IIII)V",
//                  vid_recoder.m_set_vid_param);
    GET_METHOD_ID(vid_recoder.cls, "VideoRecorder", "addAudioData", "([BI)V",
                  vid_recoder.m_add_aud_data);
    GET_METHOD_ID(vid_recoder.cls, "VideoRecorder", "addVideoData", "([BI)V",
                  vid_recoder.m_add_vid_data);
    GET_METHOD_ID(vid_recoder.cls, "VideoRecorder", "start", "()V",
                  vid_recoder.m_start);
    GET_METHOD_ID(vid_recoder.cls, "VideoRecorder", "stop", "()V",
                  vid_recoder.m_stop);

#undef GET_CLASS_ID
#undef GET_METHOD_ID
#undef GET_SMETHOD_ID
#undef GET_FIELD_ID



on_return:
    jni_detach_env();
    return status;
}

static void jni_deinit_ids() {
    JNIEnv *jni_env;
    pj_bool_t with_attach = jni_get_env(&jni_env);

    if (vid_recoder.cls) {
        (*jni_env)->DeleteGlobalRef(jni_env, vid_recoder.cls);
        vid_recoder.cls = NULL;
    }
    if (jvid_recoder) {
        (*jni_env)->DeleteGlobalRef(jni_env, jvid_recoder);
        jvid_recoder = NULL;
    }
    jni_detach_env();
}

static void JNICALL destroy(JNIEnv *env,jobject obj){
    quit = PJ_TRUE;
    pic_file_name = NULL;
    jni_deinit_ids();
}



pj_bool_t get_take_pic_running_state(){
    return is_take_pic;
}

void set_take_pic_state(pj_bool_t b){
    is_take_pic = b;
}

pj_bool_t get_vid_record_state(){
    return video_record_running;
}

pj_status_t vid_save_add_vid_data(void *buf,int size){
    static pj_bool_t inited = PJ_FALSE;
    static JNIEnv *vid_env;
    if(!vid_env) {
        jni_get_env(&vid_env);
    }
    if(quit){
        if(vid_buf && vid_output && vid_env){
            (*vid_env)->ReleaseByteArrayElements(vid_env,vid_output,vid_buf,0);
            (*vid_env)->DeleteLocalRef(vid_env, vid_output);
            jni_detach_env();
        }
        return PJ_FALSE;
    }
    if(!inited){
        inited = PJ_TRUE;
//        if(get_vid_record_state() == SDK_UP_19){
            vid_output =(*vid_env)->NewByteArray(vid_env,115200);
            vid_buf = (*vid_env)->GetByteArrayElements(vid_env,vid_output,0);
            pj_bzero(vid_buf,115200);
//        } else if(get_vid_record_state() == SDK_BELOW_19){
//            vid_output =(*vid_env)->NewByteArray(vid_env,1024);
//            vid_buf = (*vid_env)->GetByteArrayElements(vid_env,vid_output,0);
//            pj_bzero(vid_buf,1024);
//        }
    }
    pj_memcpy(vid_buf,buf,size);
    (*vid_env)->ReleaseByteArrayElements(vid_env,vid_output,vid_buf,JNI_COMMIT);
    (*vid_env)->CallVoidMethod(vid_env,jvid_recoder,vid_recoder.m_add_vid_data,vid_output,size);

    return PJ_SUCCESS;
}

//pj_status_t vid_save_add_aud_data(jbyteArray outputBuffer,int size){
//    JNIEnv *env;
//    jni_get_env(&env);
//    (*env)->CallVoidMethod(env, jvid_recoder,
//                              vid_recoder.m_add_aud_data, outputBuffer,size);
//}

pj_status_t vid_save_add_aud_data(void *buf,int size){
    static pj_bool_t aud_inited = PJ_FALSE;
    static JNIEnv* aud_env;
    if(!aud_env) {
        jni_get_env(&aud_env);
    }
    if(quit){
        if(aud_buf && aud_output && aud_env){
            (*aud_env)->ReleaseByteArrayElements(aud_env,aud_output,aud_buf,0);
            (*aud_env)->DeleteLocalRef(aud_env, aud_output);
        }
        return PJ_FALSE;
    }

    if(!aud_inited){
        aud_inited = PJ_TRUE;
        aud_output =(*aud_env)->NewByteArray(aud_env,2048);
        aud_buf = (*aud_env)->GetByteArrayElements(aud_env,aud_output,0);
        pj_bzero(aud_buf,2048);
    }
    pj_memcpy(aud_buf,buf,size);
    (*aud_env)->ReleaseByteArrayElements(aud_env,aud_output,aud_buf,JNI_COMMIT);
    (*aud_env)->CallVoidMethod(aud_env, jvid_recoder,
                             vid_recoder.m_add_aud_data, aud_output,size);
    return PJ_SUCCESS;
}

/* Register native function */
pj_bool_t vid_save_register(){
    JNIEnv *jni_env;
    jni_get_env(&jni_env);

#define GET_CLASS(class_path, class_name, cls) \
    cls = (*jni_env)->FindClass(jni_env, class_path); \
    if (cls == NULL || (*jni_env)->ExceptionCheck(jni_env)) { \
    (*jni_env)->ExceptionClear(jni_env); \
        PJ_LOG(3, (THIS_FILE, "[JNI] Unable to find class '" \
                  class_name "'")); \
    } else { \
        jclass tmp = cls; \
    cls = (jclass)(*jni_env)->NewGlobalRef(jni_env, tmp); \
    (*jni_env)->DeleteLocalRef(jni_env, tmp); \
    if (cls == NULL) { \
        PJ_LOG(3, (THIS_FILE, "[JNI] Unable to get global ref for " \
                  "class '" class_name "'")); \
    } \
    }

    GET_CLASS(VIDEO_RECODER_CLASS_PATH, "VideoRecorder", vid_recoder.cls);

    JNINativeMethod m[] = {
            {"init","()V",&init},
            {"take_pic", "(Ljava/lang/String;)V", (void *) &take_pic},
            {"start_record", "(I)V", (void *) &start},
            {"stop_record", "()V", (void *) &stop},
            {"destroy", "()V", (void *) &destroy}};
    if ((*jni_env)->RegisterNatives(jni_env, vid_recoder.cls, m, 5)) {
        PJ_LOG(3, (THIS_FILE, "[JNI] Failed in registering native "
                              "function 'OnGetFrame()'"));
    }

    return PJ_TRUE;
}



#endif    /* PJMEDIA_VIDEO_DEV_HAS_ANDROID */


