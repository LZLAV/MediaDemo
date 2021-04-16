//
// Created by SH Qiu on 2020-09-07.
//

#include "util.h"
#include <pjmedia-videodev/videodev_imp.h>
#include <pj/assert.h>
#include <pj/log.h>
#include <pj/math.h>
#include <pj/os.h>

#if defined(PJMEDIA_HAS_VIDEO) && PJMEDIA_HAS_VIDEO != 0

#include <jni.h>
#include <pjmedia/transport.h>
#include <endian.h>
#include <linux/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pjmedia/clock.h>

/* Android MediaCodec: */
#include <media/NdkMediaCodec.h>
#include <media/NdkMediaFormat.h>
#include <sys/time.h>

#include <pthread.h>
#include <pjsua-lib/pjsua.h>
#include <pjsua-lib/pjsua_internal.h>

#define THIS_FILE		"hytera_dev.c"

#define HYTERA_DEV_DEBUG
#ifdef HYTERA_DEV_DEBUG
#define HYTERA_DEV_LOG(format, args...) do { \
                         PJ_LOG(5, (THIS_FILE, "[%s][%d]: " format, __FUNCTION__, __LINE__, ##args));  \
                        } while (0);
#else
#   define HYTERA_DEV_LOG(format, args...)
#endif


/* Default video params */
#define DEFAULT_CLOCK_RATE	90000
#define DEFAULT_WIDTH		352
#define DEFAULT_HEIGHT		288
#define DEFAULT_FPS		15
#define ALIGN16(x)		((((x)+15) >> 4) << 4)

/* Define whether we should maintain the aspect ratio when rotating the image.
 * For more details, please refer to util.h.
 */
#define MAINTAIN_ASPECT_RATIO 	PJ_TRUE

/* Format map info */
typedef struct hytera_fmt_map
{
    pjmedia_format_id   fmt_id;
    pj_uint32_t		and_fmt_id;
} hytera_fmt_map;


/* Format map.
 * Note: it seems that most of Android devices don't support I420, while
 * unfortunately, our converter (libyuv based) only support I420 & RGBA,
 * so in this case, we'd just pretend that we support I420 and we'll do
 * the NV21/YV12 -> I420 conversion here.
 */
static hytera_fmt_map fmt_map[] =
        {
            {PJMEDIA_FORMAT_NV21, 0x00000011},
            {PJMEDIA_FORMAT_YV12, 0x32315659},
            {PJMEDIA_FORMAT_I420, 0x00000023}, /* YUV_420_888 */
        };

/* Device info */
typedef struct hytera_dev_info {

    pjmedia_vid_dev_info	 info;		/**< Base info         */

    unsigned			 dev_idx;	/**< Original dev ID   */
    pj_bool_t			 facing;	/**< Front/back camera?*/
    unsigned			 sup_size_cnt;	/**< # of supp'd size  */
    pjmedia_rect_size		*sup_size;	/**< Supported size    */
    unsigned			 sup_fps_cnt;	/**< # of supp'd FPS   */
    pjmedia_rect_size		*sup_fps;	/**< Supported FPS     */
    pj_bool_t			 has_yv12;	/**< Support YV12?     */
    pj_bool_t			 has_nv21;	/**< Support NV21?     */
    pj_bool_t			 forced_i420;	/**< Support I420 with
						     conversion		*/
} hytera_dev_info;

/* Video factory */
typedef struct hytera_factory
{
    pjmedia_vid_dev_factory	 base;		/**< Base factory      */
    pj_pool_t			*pool;		/**< Memory pool       */
    pj_pool_factory		*pf;		/**< Pool factory      */

    pj_pool_t			*dev_pool;	/**< Device list pool  */
    unsigned			 dev_count;	/**< Device count      */
    hytera_dev_info		*dev_info;	/**< Device info list  */
} hytera_factory;

/* Video stream. */
typedef struct hytera_stream
{
    pjmedia_vid_dev_stream  base;		/**< Base stream       */
    pjmedia_vid_dev_param   param;		/**< Settings	       */
    pj_pool_t		        *pool;		/**< Memory pool       */
    hytera_factory		    *factory;   /**< Factory           */

    pjmedia_vid_dev_cb	    vid_cb;		/**< Stream callback   */
    void		   *user_data;          /**< Application data  */
    pj_bool_t		    is_running;		/**< Stream running?   */

    jobject		    jcam;		/**< PjCamera instance */

    pj_timestamp            frame_ts;		/**< Current timestamp */
    unsigned                ts_inc;		/**< Timestamp interval*/
    unsigned		    convert_to_i420;	/**< Need to convert to I420?
						     0: no
						     1: from NV21
						     2: from YV12	*/

    /** NV21/YV12 -> I420 Conversion buffer  */
    pj_uint8_t		   *convert_buf;
    pjmedia_rect_size	    cam_size;

    /** Converter to rotate frame  */
    pjmedia_vid_dev_conv    conv;

    /** Frame format param for NV21/YV12 -> I420 conversion */
    pjmedia_video_apply_fmt_param vafp;

    unsigned fps_num; // 帧率
} hytera_stream;

typedef struct hytera_camera_data {

    pj_sock_t server_socket_fd;
    pj_sock_t client_socket_fd;
    pj_bool_t socket_server_quit_flag;

    AMediaCodec *dec;
    pj_uint8_t	*dec_buf;
    unsigned	dec_buf_size;

    pj_uint8_t *rec_whole_frame_buf;     // 一帧完整视频 buf
    unsigned   rec_whole_frame_buf_size;

    pj_bool_t   first_h264_frame;

    pthread_t hytera_thread;
} hytera_camera_data;

static hytera_camera_data camera_data_var;

/* Prototypes */
static pj_status_t hytera_factory_init(pjmedia_vid_dev_factory *f);
static pj_status_t hytera_factory_destroy(pjmedia_vid_dev_factory *f);
static pj_status_t hytera_factory_refresh(pjmedia_vid_dev_factory *f);
static unsigned    hytera_factory_get_dev_count(pjmedia_vid_dev_factory *f);
static pj_status_t hytera_factory_get_dev_info(pjmedia_vid_dev_factory *f,
                                            unsigned index,
                                            pjmedia_vid_dev_info *info);
static pj_status_t hytera_factory_default_param(pj_pool_t *pool,
                                             pjmedia_vid_dev_factory *f,
                                             unsigned index,
                                             pjmedia_vid_dev_param *param);
static pj_status_t hytera_factory_create_stream(
        pjmedia_vid_dev_factory *f,
        pjmedia_vid_dev_param *param,
        const pjmedia_vid_dev_cb *cb,
        void *user_data,
        pjmedia_vid_dev_stream **p_vid_strm);


static pj_status_t hytera_stream_get_param(pjmedia_vid_dev_stream *strm,
                                        pjmedia_vid_dev_param *param);
static pj_status_t hytera_stream_get_cap(pjmedia_vid_dev_stream *strm,
                                      pjmedia_vid_dev_cap cap,
                                      void *value);
static pj_status_t hytera_stream_set_cap(pjmedia_vid_dev_stream *strm,
                                      pjmedia_vid_dev_cap cap,
                                      const void *value);
static pj_status_t hytera_stream_start(pjmedia_vid_dev_stream *strm);
static pj_status_t hytera_stream_stop(pjmedia_vid_dev_stream *strm);
static pj_status_t hytera_stream_destroy(pjmedia_vid_dev_stream *strm);

/**********************************************************************************
 * hytera camera data methods
 */
static int hytera_camera_data_call_back(void *userData);

static pj_status_t hytera_camera_data_init();

static pj_status_t hytera_camera_data_socket_init();
static pj_status_t hytera_camera_data_socket_deinit();

static pj_status_t hytera_camera_data_deinit();

/**
 * 海能达设备套接字 accept 超时处理，避免调用 accept 时造成阻塞
 */
static pj_status_t hytera_camera_data_socket_accept(pj_sock_t serverfd, pj_sock_t *clientfd);
static void hytera_camera_data_socket_server_recv_handle(hytera_stream *strm);

/**
 * NDK decodec  AMediaCodec
 */
static pj_status_t hytera_camera_data_mediacodec_decoder_init(unsigned width, unsigned height, int fps);
static pj_status_t hytera_camera_data_mediacodec_decode(hytera_stream *strm, char *inBuf, int inBufSize);
static pj_status_t hytera_camera_data_mediacodec_decoder_deinit();

/* Operations */
static pjmedia_vid_dev_factory_op factory_op =
{
    &hytera_factory_init,
    &hytera_factory_destroy,
    &hytera_factory_get_dev_count,
    &hytera_factory_get_dev_info,
    &hytera_factory_default_param,
    &hytera_factory_create_stream,
    &hytera_factory_refresh
};

static pjmedia_vid_dev_stream_op stream_op =
{
    &hytera_stream_get_param,
    &hytera_stream_get_cap,
    &hytera_stream_set_cap,
    &hytera_stream_start,
    NULL,
    NULL,
    &hytera_stream_stop,
    &hytera_stream_destroy
};

/****************************************************************************
 * JNI stuff
 */

extern JavaVM *pj_jni_jvm;
#define PJ_HYTERA_CAMERA_PATH "org/pjsip/hytera/PjHyteraCamera"
#define PJ_CAMERA_INFO_CLASS_PATH	"org/pjsip/PjCameraInfo"

static struct jni_objs_t
{
    struct {
        jclass cls;
        jmethodID	 m_init;
        jmethodID	 m_start;
        jmethodID	 m_stop;
    } cam;

    struct {
        jclass		 cls;
        jmethodID	 m_get_cnt;
        jmethodID	 m_get_info;
        jfieldID	 f_facing;
        jfieldID	 f_orient;
        jfieldID	 f_sup_size;
        jfieldID	 f_sup_fmt;
        jfieldID	 f_sup_fps;
    } cam_info;
} jobjs;

static pj_bool_t jni_get_env(JNIEnv **jni_env)
{
    pj_bool_t with_attach = PJ_FALSE;
    if ((*pj_jni_jvm)->GetEnv(pj_jni_jvm, (void **)jni_env,
                              JNI_VERSION_1_4) < 0)
    {
        if ((*pj_jni_jvm)->AttachCurrentThread(pj_jni_jvm, jni_env, NULL) < 0)
        {
            *jni_env = NULL;
        } else {
            with_attach = PJ_TRUE;
        }
    }

    return with_attach;
}

static void jni_detach_env(pj_bool_t need_detach)
{
    if (need_detach)
        (*pj_jni_jvm)->DetachCurrentThread(pj_jni_jvm);
}

/* Get Java object IDs (via FindClass, GetMethodID, GetFieldID, etc).
 * Note that this function should be called from library-loader thread,
 * otherwise FindClass, etc, may fail, see:
 * http://developer.android.com/training/articles/perf-jni.html#faq_FindClass
 */
static pj_status_t jni_init_ids() {

    JNIEnv *jni_env;
    pj_status_t status = PJ_SUCCESS;
    pj_bool_t with_attach = jni_get_env(&jni_env);

#define GET_CLASS(class_path, class_name, cls) \
    cls = (*jni_env)->FindClass(jni_env, class_path); \
    if (cls == NULL || (*jni_env)->ExceptionCheck(jni_env)) { \
	(*jni_env)->ExceptionClear(jni_env); \
        PJ_LOG(3, (THIS_FILE, "[JNI] Unable to find class '" \
			      class_name "'")); \
        status = PJMEDIA_EVID_SYSERR; \
        goto on_return; \
    } else { \
        jclass tmp = cls; \
	cls = (jclass)(*jni_env)->NewGlobalRef(jni_env, tmp); \
	(*jni_env)->DeleteLocalRef(jni_env, tmp); \
	if (cls == NULL) { \
	    PJ_LOG(3, (THIS_FILE, "[JNI] Unable to get global ref for " \
				  "class '" class_name "'")); \
	    status = PJMEDIA_EVID_SYSERR; \
	    goto on_return; \
	} \
    }
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

    /* PjHyteraCamera class info */
    GET_CLASS(PJ_HYTERA_CAMERA_PATH, "PjHyteraCamera", jobjs.cam.cls);
    GET_METHOD_ID(jobjs.cam.cls, "PjHyteraCamera", "<init>", "(IIII)V", jobjs.cam.m_init);
    GET_METHOD_ID(jobjs.cam.cls, "PjHyteraCamera", "start", "()V", jobjs.cam.m_start);
    GET_METHOD_ID(jobjs.cam.cls, "PjHyteraCamera", "stop", "()V", jobjs.cam.m_stop);

    /* PjCameraInfo class info */
    GET_CLASS(PJ_CAMERA_INFO_CLASS_PATH, "PjCameraInfo", jobjs.cam_info.cls);
    GET_SMETHOD_ID(jobjs.cam_info.cls, "PjCameraInfo", "GetCameraCount", "()I",
                   jobjs.cam_info.m_get_cnt);
    GET_SMETHOD_ID(jobjs.cam_info.cls, "PjCameraInfo", "GetCameraInfo",
                   "(I)L" PJ_CAMERA_INFO_CLASS_PATH ";",
                   jobjs.cam_info.m_get_info);
    GET_FIELD_ID(jobjs.cam_info.cls, "PjCameraInfo", "facing", "I",
                 jobjs.cam_info.f_facing);
    GET_FIELD_ID(jobjs.cam_info.cls, "PjCameraInfo", "orient", "I",
                 jobjs.cam_info.f_orient);
    GET_FIELD_ID(jobjs.cam_info.cls, "PjCameraInfo", "supportedSize", "[I",
                 jobjs.cam_info.f_sup_size);
    GET_FIELD_ID(jobjs.cam_info.cls, "PjCameraInfo", "supportedFormat", "[I",
                 jobjs.cam_info.f_sup_fmt);
    GET_FIELD_ID(jobjs.cam_info.cls, "PjCameraInfo", "supportedFps1000", "[I",
                 jobjs.cam_info.f_sup_fps);

#undef GET_CLASS_ID
#undef GET_METHOD_I
#undef GET_SMETHOD_ID
#undef GET_FIELD_ID

on_return:
    jni_detach_env(with_attach);
    return status;
}

static void jni_deinit_ids()
{
    JNIEnv *jni_env;
    pj_bool_t with_attach = jni_get_env(&jni_env);

    if (jobjs.cam.cls) {
        (*jni_env)->DeleteGlobalRef(jni_env, jobjs.cam.cls);
        jobjs.cam.cls = NULL;
    }

    if (jobjs.cam_info.cls) {
        (*jni_env)->DeleteGlobalRef(jni_env, jobjs.cam_info.cls);
        jobjs.cam_info.cls = NULL;
    }

    jni_detach_env(with_attach);
}

/****************************************************************************
 * Helper functions
 */
static pjmedia_format_id hytera_fmt_to_pj(pj_uint32_t fmt)
{
    unsigned i;
    for (i = 0; i < PJ_ARRAY_SIZE(fmt_map); i++) {
        if (fmt_map[i].and_fmt_id == fmt)
            return fmt_map[i].fmt_id;
    }
    return 0;
}

static pj_uint32_t pj_fmt_to_hytera(pjmedia_format_id fmt)
{
    unsigned i;
    for (i = 0; i < PJ_ARRAY_SIZE(fmt_map); i++) {
        if (fmt_map[i].fmt_id == fmt)
            return fmt_map[i].and_fmt_id;
    }
    return 0;
}


/****************************************************************************
 * Factory operations
 */
/*
 * Init and_ video driver.
 */
pjmedia_vid_dev_factory* pjmedia_hytera_factory(pj_pool_factory *pf)
{
    hytera_factory *f;
    pj_pool_t *pool;

    pool = pj_pool_create(pf, "hytera_video", 512, 512, NULL);
    f = PJ_POOL_ZALLOC_T(pool, hytera_factory);
    f->pf = pf;
    f->pool = pool;
    f->base.op = &factory_op;
    f->dev_pool = pj_pool_create(pf, "hytera_video_dev", 512, 512, NULL);

    return &f->base;
}


static pj_status_t hytera_factory_init(pjmedia_vid_dev_factory *ff) {

    pj_status_t status;

    status = jni_init_ids();
    if (status != PJ_SUCCESS)
        return status;

    status = hytera_factory_refresh(ff);
    if (status != PJ_SUCCESS)
        return status;

    return PJ_SUCCESS;
}

static pj_status_t hytera_factory_destroy(pjmedia_vid_dev_factory *ff) {

    hytera_factory *f = (hytera_factory*)ff;
    pj_pool_t *pool;

    jni_deinit_ids();

    pool = f->pool;
    f->pool = NULL;
    if (pool)
        pj_pool_release(pool);

    pool = f->dev_pool;
    f->dev_pool = NULL;
    if (pool)
        pj_pool_release(pool);

    return PJ_SUCCESS;
}

static pj_status_t hytera_factory_refresh(pjmedia_vid_dev_factory *ff) {

    hytera_factory *f = (hytera_factory*)ff;
    pj_status_t status = PJ_SUCCESS;

    JNIEnv *jni_env;
    pj_bool_t with_attach, found_front = PJ_FALSE;
    int i, dev_count = 0;

    /* Clean up device info and pool */
    f->dev_count = 0;
    pj_pool_reset(f->dev_pool);

    with_attach = jni_get_env(&jni_env);

    /* dev_count = PjCameraInfo::GetCameraCount() */
    dev_count = 1;

    if (dev_count < 0) {
        PJ_LOG(3, (THIS_FILE, "Failed to get camera count"));
        status = PJMEDIA_EVID_SYSERR;
        goto on_return;
    }

    /* Start querying device info */
    f->dev_info = (hytera_dev_info*)
            pj_pool_calloc(f->dev_pool, dev_count,
                           sizeof(hytera_dev_info));

    for (i = 0; i < dev_count; i++) {

        hytera_dev_info *adi = &f->dev_info[f->dev_count];
        pjmedia_vid_dev_info *vdi = &adi->info;

        int facing, max_fmt_cnt = PJMEDIA_VID_DEV_INFO_FMT_CNT;

        /* Get camera facing: 0=back 1=front */
        facing = 0;

        /* Set device ID, direction, and has_callback info */
        adi->dev_idx = i;
        vdi->id = f->dev_count;
        vdi->dir = PJMEDIA_DIR_CAPTURE;
        vdi->has_callback = PJ_TRUE;
        vdi->caps = PJMEDIA_VID_DEV_CAP_SWITCH |
                    PJMEDIA_VID_DEV_CAP_ORIENTATION;

        /* Set driver & name info */
        pj_ansi_strncpy(vdi->driver, "Android", sizeof(vdi->driver));
        adi->facing = facing;
        if (facing == 0) {
            pj_ansi_strncpy(vdi->name, "Back camera", sizeof(vdi->name));
        } else {
            pj_ansi_strncpy(vdi->name, "Front camera", sizeof(vdi->name));
        }

        /* Get supported sizes */
        {
            jsize j;
            jsize cnt = 46;
            jint sizes[46] = {1920, 1080, 1600, 1200, 1440, 1080, 1280, 960,
                               1280, 768,  1280, 720,  1200, 1200, 1024, 768,
                               800,  600,  864,  480,  800,  480,  720,  480,
                               640,  480,  640,  360,  480,  640,  480,  360,
                               480,  320,  352,  288,  320,  240,  240,  320,
                               176,  144,  160,  120,  144,  176};

            adi->sup_size_cnt = cnt/2;
            adi->sup_size = pj_pool_calloc(f->dev_pool, adi->sup_size_cnt,
                                           sizeof(adi->sup_size[0]));
            for (j = 0; j < adi->sup_size_cnt; j++) {
                adi->sup_size[j].w = sizes[j*2];
                adi->sup_size[j].h = sizes[j*2+1];
            }
        }

        /* Get supported formats */
        {
            pj_bool_t has_i420 = PJ_FALSE;
            int k;
            jsize j;

            jsize cnt = 2;
            jint fmts[2] = {842094169, 17};

            for (j = 0; j < cnt; j++) {
                pjmedia_format_id fmt = hytera_fmt_to_pj((pj_uint32_t)fmts[j]);

                /* Make sure we recognize this format */
                if (fmt == 0)
                    continue;

                /* Check formats for I420 conversion */
                if (fmt == PJMEDIA_FORMAT_I420) has_i420 = PJ_TRUE;
                else if (fmt == PJMEDIA_FORMAT_YV12) adi->has_yv12 = PJ_TRUE;
                else if (fmt == PJMEDIA_FORMAT_NV21) adi->has_nv21 = PJ_TRUE;
            }

            /* Always put I420/IYUV and in the first place, for better
             * compatibility.
             */
            adi->forced_i420 = !has_i420;
            for (k = 0; k < adi->sup_size_cnt &&
                        vdi->fmt_cnt < max_fmt_cnt-1; k++)
            {
                /* Landscape video */
                pjmedia_format_init_video(&vdi->fmt[vdi->fmt_cnt++],
                                          PJMEDIA_FORMAT_I420,
                                          adi->sup_size[k].w,
                                          adi->sup_size[k].h,
                                          DEFAULT_FPS, 1);
                /* Portrait video */
                pjmedia_format_init_video(&vdi->fmt[vdi->fmt_cnt++],
                                          PJMEDIA_FORMAT_I420,
                                          adi->sup_size[k].h,
                                          adi->sup_size[k].w,
                                          DEFAULT_FPS, 1);
            }

            /* YV12 */
            if (adi->has_yv12) {
                for (k = 0; k < adi->sup_size_cnt &&
                            vdi->fmt_cnt < max_fmt_cnt-1; k++)
                {
                    /* Landscape video */
                    pjmedia_format_init_video(&vdi->fmt[vdi->fmt_cnt++],
                                              PJMEDIA_FORMAT_YV12,
                                              adi->sup_size[k].w,
                                              adi->sup_size[k].h,
                                              DEFAULT_FPS, 1);
                    /* Portrait video */
                    pjmedia_format_init_video(&vdi->fmt[vdi->fmt_cnt++],
                                              PJMEDIA_FORMAT_YV12,
                                              adi->sup_size[k].h,
                                              adi->sup_size[k].w,
                                              DEFAULT_FPS, 1);
                }
            }

            /* NV21 */
            if (adi->has_nv21) {
                for (k = 0; k < adi->sup_size_cnt &&
                            vdi->fmt_cnt < max_fmt_cnt-1; k++)
                {
                    /* Landscape video */
                    pjmedia_format_init_video(&vdi->fmt[vdi->fmt_cnt++],
                                              PJMEDIA_FORMAT_NV21,
                                              adi->sup_size[k].w,
                                              adi->sup_size[k].h,
                                              DEFAULT_FPS, 1);
                    /* Portrait video */
                    pjmedia_format_init_video(&vdi->fmt[vdi->fmt_cnt++],
                                              PJMEDIA_FORMAT_NV21,
                                              adi->sup_size[k].h,
                                              adi->sup_size[k].w,
                                              DEFAULT_FPS, 1);
                }
            }
        }


        /* If this is front camera, set it as first/default (if not yet) */
        if (facing == 1) {
            if (!found_front && f->dev_count > 0) {
                /* Swap this front cam info with one whose idx==0 */
                hytera_dev_info tmp_adi;
                pj_memcpy(&tmp_adi, &f->dev_info[0], sizeof(tmp_adi));
                pj_memcpy(&f->dev_info[0], adi, sizeof(tmp_adi));
                pj_memcpy(adi, &tmp_adi, sizeof(tmp_adi));
                f->dev_info[0].info.id = 0;
                f->dev_info[f->dev_count].info.id = f->dev_count;
            }
            found_front = PJ_TRUE;
        }

        f->dev_count++;
    }

    PJ_LOG(4, (THIS_FILE,
            "Android video capture initialized with %d device(s):",
            f->dev_count));

on_return:
    jni_detach_env(with_attach);
    return status;
}

static unsigned hytera_factory_get_dev_count(pjmedia_vid_dev_factory *ff) {

    hytera_factory *f = (hytera_factory*)ff;
    return f->dev_count;
}

static pj_status_t hytera_factory_get_dev_info(pjmedia_vid_dev_factory *f,
                                               unsigned index,
                                               pjmedia_vid_dev_info *info) {

    hytera_factory *cf = (hytera_factory*)f;

    PJ_ASSERT_RETURN(index < cf->dev_count, PJMEDIA_EVID_INVDEV);

    pj_memcpy(info, &cf->dev_info[index].info, sizeof(*info));

    return PJ_SUCCESS;
}

static pj_status_t hytera_factory_default_param(pj_pool_t *pool,
                                                pjmedia_vid_dev_factory *f,
                                                unsigned index,
                                                pjmedia_vid_dev_param *param)  {

    hytera_factory *cf = (hytera_factory*)f;
    hytera_dev_info *di = &cf->dev_info[index];

    PJ_ASSERT_RETURN(index < cf->dev_count, PJMEDIA_EVID_INVDEV);

    PJ_UNUSED_ARG(pool);

    pj_bzero(param, sizeof(*param));
    param->dir = PJMEDIA_DIR_CAPTURE;
    param->cap_id = index;
    param->rend_id = PJMEDIA_VID_INVALID_DEV;
    param->flags = PJMEDIA_VID_DEV_CAP_FORMAT;
    param->clock_rate = DEFAULT_CLOCK_RATE;
    pj_memcpy(&param->fmt, &di->info.fmt[0], sizeof(param->fmt));

    return PJ_SUCCESS;
}

static pj_status_t hytera_factory_create_stream (
        pjmedia_vid_dev_factory *ff,
        pjmedia_vid_dev_param *param,
        const pjmedia_vid_dev_cb *cb,
        void *user_data,
        pjmedia_vid_dev_stream **p_vid_strm) {

    hytera_factory *f = (hytera_factory*)ff;
    pj_pool_t *pool;
    hytera_stream *strm;
    hytera_dev_info *adi;
    const pjmedia_video_format_detail *vfd;
    const pjmedia_video_format_info *vfi;
    pjmedia_video_apply_fmt_param vafp;
    pj_uint32_t and_fmt = 0;
    unsigned convert_to_i420 = 0;
    pj_status_t status = PJ_SUCCESS;

    JNIEnv *jni_env;
    pj_bool_t with_attach;
    jobject jcam;

    PJ_ASSERT_RETURN(f && param && p_vid_strm, PJ_EINVAL);
    PJ_ASSERT_RETURN(param->fmt.type == PJMEDIA_TYPE_VIDEO &&
                     param->fmt.detail_type == PJMEDIA_FORMAT_DETAIL_VIDEO &&
                     param->dir == PJMEDIA_DIR_CAPTURE,
                     PJ_EINVAL);

    pj_bzero(&vafp, sizeof(vafp));
    adi = &f->dev_info[param->cap_id];
    vfd = pjmedia_format_get_video_format_detail(&param->fmt, PJ_TRUE);
    vfi = pjmedia_get_video_format_info(NULL, param->fmt.id);

    if (param->fmt.id == PJMEDIA_FORMAT_I420 && adi->forced_i420) {
        /* Not really support I420, need to convert it from YV12/NV21 */
        if (adi->has_nv21) {
            and_fmt = pj_fmt_to_hytera(PJMEDIA_FORMAT_NV21);
            convert_to_i420 = 1;
        } else if (adi->has_yv12) {
            and_fmt = pj_fmt_to_hytera(PJMEDIA_FORMAT_YV12);
            convert_to_i420 = 2;
        } else
            pj_assert(!"Bug!");
    } else {
        and_fmt = pj_fmt_to_hytera(param->fmt.id);
    }
    if (!vfi || !and_fmt)
        return PJMEDIA_EVID_BADFORMAT;

    vafp.size = vfd->size;
    if (vfi->apply_fmt(vfi, &vafp) != PJ_SUCCESS)
        return PJMEDIA_EVID_BADFORMAT;

    /* Create and Initialize stream descriptor */
    pool = pj_pool_create(f->pf, "hytera-dev", 512, 512, NULL);
    PJ_ASSERT_RETURN(pool != NULL, PJ_ENOMEM);

    strm = PJ_POOL_ZALLOC_T(pool, hytera_stream);
    pj_memcpy(&strm->param, param, sizeof(*param));
    strm->pool = pool;
    strm->factory = f;
    pj_memcpy(&strm->vid_cb, cb, sizeof(*cb));
    strm->user_data = user_data;
    pj_memcpy(&strm->vafp, &vafp, sizeof(vafp));
    strm->ts_inc = PJMEDIA_SPF2(param->clock_rate, &vfd->fps, 1);

    /* Allocate buffer for YV12 -> I420 conversion */
    if (convert_to_i420) {
        pj_assert(vfi->plane_cnt > 1);
        strm->convert_to_i420 = convert_to_i420;
        strm->convert_buf = pj_pool_alloc(pool, vafp.plane_bytes[1]);
    }

    /* Native preview */
    if (param->flags & PJMEDIA_VID_DEV_CAP_INPUT_PREVIEW) {
    }

    with_attach = jni_get_env(&jni_env);

    /* Instantiate PjHyteraCamera */
    strm->cam_size.w = (vfd->size.w > vfd->size.h? vfd->size.w: vfd->size.h);
    strm->cam_size.h = (vfd->size.w > vfd->size.h? vfd->size.h: vfd->size.w);
    strm->fps_num = vfd->fps.num;
    jcam = (*jni_env)->NewObject(jni_env, jobjs.cam.cls, jobjs.cam.m_init,
                                 strm->cam_size.w,	/* w */
                                 strm->cam_size.h,	/* h */
                                 vfd->avg_bps,		/* bps */
                                 vfd->fps.num*1000/
                                 vfd->fps.denum  	/* fps */
                                 );

    if (jcam == NULL) {
        PJ_LOG(3, (THIS_FILE, "Unable to create PjHyteraCamera instance"));
        status = PJMEDIA_EVID_SYSERR;
        goto on_return;
    }

    strm->jcam = (jobject)(*jni_env)->NewGlobalRef(jni_env, jcam);
    (*jni_env)->DeleteLocalRef(jni_env, jcam);
    if (strm->jcam == NULL) {
        PJ_LOG(3, (THIS_FILE, "Unable to create global ref to PjCamera"));
        status = PJMEDIA_EVID_SYSERR;
        goto on_return;
    }

    /* Video orientation.
     * If we send in portrait, we need to set up orientation converter
     * as well.
     */
    if ((param->flags & PJMEDIA_VID_DEV_CAP_ORIENTATION) ||
        (vfd->size.h > vfd->size.w))
    {
        if (param->orient == PJMEDIA_ORIENT_UNKNOWN)
            param->orient = PJMEDIA_ORIENT_NATURAL;
        hytera_stream_set_cap(&strm->base, PJMEDIA_VID_DEV_CAP_ORIENTATION,
                           &param->orient);
    }

    int ret_thrd = pthread_create(&camera_data_var.hytera_thread, NULL, (void *)hytera_camera_data_call_back, (
            void *)strm);
    if (ret_thrd != 0) {

        PJ_LOG(3, (THIS_FILE, "pthread_create failed")); // 创建线程失败
        status = PJ_EINVAL;
        goto on_return;
    }

    PJ_LOG(3, (THIS_FILE, "hytera_factory_create_stream success"));

on_return:
    jni_detach_env(with_attach);

    /* Success */
    if (status == PJ_SUCCESS) {
        strm->base.op = &stream_op;
        *p_vid_strm = &strm->base;
    }

    return status;
}


/****************************************************************************
 * Stream operations
 */

static pj_status_t hytera_stream_get_param(pjmedia_vid_dev_stream *s,
                                           pjmedia_vid_dev_param *pi) {

    hytera_stream *strm = (hytera_stream*)s;

    PJ_ASSERT_RETURN(strm && pi, PJ_EINVAL);

    pj_memcpy(pi, &strm->param, sizeof(*pi));

    if (hytera_stream_get_cap(s, PJMEDIA_VID_DEV_CAP_OUTPUT_WINDOW,
                           &pi->window) == PJ_SUCCESS)
    {
        pi->flags |= PJMEDIA_VID_DEV_CAP_OUTPUT_WINDOW;
    }

    return PJ_SUCCESS;
}

static pj_status_t hytera_stream_get_cap(pjmedia_vid_dev_stream *s,
                                         pjmedia_vid_dev_cap cap,
                                         void *pval) {

    hytera_stream *strm = (hytera_stream*)s;

    PJ_UNUSED_ARG(strm);
    PJ_UNUSED_ARG(cap);
    PJ_UNUSED_ARG(pval);

    return PJMEDIA_EVID_INVCAP;
}

static pj_status_t hytera_stream_set_cap(pjmedia_vid_dev_stream *s,
                                         pjmedia_vid_dev_cap cap,
                                         const void *pval) {

    hytera_stream *strm = (hytera_stream*)s;
    pj_status_t status = PJ_SUCCESS;

    PJ_ASSERT_RETURN(s && pval, PJ_EINVAL);
    switch (cap) {

        case PJMEDIA_VID_DEV_CAP_ORIENTATION:
        {
            pjmedia_orient orient = *(pjmedia_orient *)pval;
            pjmedia_orient eff_ori;
            hytera_dev_info *adi;

            pj_assert(orient >= PJMEDIA_ORIENT_UNKNOWN &&
                      orient <= PJMEDIA_ORIENT_ROTATE_270DEG);

            if (orient == PJMEDIA_ORIENT_UNKNOWN)
                return PJ_EINVAL;

            pj_memcpy(&strm->param.orient, pval,
                      sizeof(strm->param.orient));

            if (!strm->conv.conv) {
                status = pjmedia_vid_dev_conv_create_converter(
                        &strm->conv, strm->pool,
                        &strm->param.fmt,
                        strm->cam_size,
                        strm->param.fmt.det.vid.size,
                        PJ_TRUE,
                        MAINTAIN_ASPECT_RATIO);

                if (status != PJ_SUCCESS)
                    return status;
            }

            eff_ori = strm->param.orient;
            adi = &strm->factory->dev_info[strm->param.cap_id];
            /* Normalize the orientation for back-facing camera */
            if (!adi->facing) {
                if (eff_ori == PJMEDIA_ORIENT_ROTATE_90DEG)
                    eff_ori = PJMEDIA_ORIENT_ROTATE_270DEG;
                else if (eff_ori == PJMEDIA_ORIENT_ROTATE_270DEG)
                    eff_ori = PJMEDIA_ORIENT_ROTATE_90DEG;
            }
            pjmedia_vid_dev_conv_set_rotation(&strm->conv, eff_ori);

            PJ_LOG(4, (THIS_FILE, "Video capture orientation set to %d",
                    strm->param.orient));

            break;
        }

        default:
            status = PJMEDIA_EVID_INVCAP;
            break;
    }

    return status;
}

static pj_status_t hytera_stream_start(pjmedia_vid_dev_stream *s) {

    hytera_stream *strm = (hytera_stream*)s;
    JNIEnv *jni_env;
    pj_bool_t with_attach;
    pj_status_t status = PJ_SUCCESS;

    PJ_LOG(4, (THIS_FILE, "Starting Android camera stream"));

    with_attach = jni_get_env(&jni_env);

    /* Call PjHyteraCamera::start() method */
    (*jni_env)->CallVoidMethod(jni_env, strm->jcam, jobjs.cam.m_start);

    strm->is_running = PJ_TRUE;

    jni_detach_env(with_attach);
    return status;
}

static pj_status_t hytera_stream_stop(pjmedia_vid_dev_stream *s) {

    hytera_stream *strm = (hytera_stream*)s;
    JNIEnv *jni_env;
    pj_bool_t with_attach;
    pj_status_t status = PJ_SUCCESS;

    PJ_ASSERT_RETURN(strm != NULL, PJ_EINVAL);

    PJ_LOG(4, (THIS_FILE, "Stopping Android camera stream"));

    with_attach = jni_get_env(&jni_env);

    if (strm->is_running == PJ_TRUE) {

        /* Call PjHyteraCamera::stop() method */
        (*jni_env)->CallVoidMethod(jni_env, strm->jcam, jobjs.cam.m_stop);
    }

    strm->is_running = PJ_FALSE;
    camera_data_var.socket_server_quit_flag = PJ_TRUE;

    jni_detach_env(with_attach);

    return status;
}

static pj_status_t hytera_stream_destroy(pjmedia_vid_dev_stream *s) {

    hytera_stream *strm = (hytera_stream*)s;
    JNIEnv *jni_env;
    pj_bool_t with_attach;

    PJ_ASSERT_RETURN(strm != NULL, PJ_EINVAL);

    with_attach = jni_get_env(&jni_env);

    if (strm->is_running)
        hytera_stream_stop(s);

    if (strm->jcam) {
        (*jni_env)->DeleteGlobalRef(jni_env, strm->jcam);
        strm->jcam = NULL;
    }

    jni_detach_env(with_attach);

    pjmedia_vid_dev_conv_destroy_converter(&strm->conv);

    if (strm->pool)
        pj_pool_release(strm->pool);

    PJ_LOG(4, (THIS_FILE, "Android camera stream destroyed"));
    return PJ_SUCCESS;
}

/**********************************************************************************
 * hytera camera data methods
 */

static int hytera_camera_data_call_back(void *userData) {

    pj_status_t status;
    hytera_stream *strm = (hytera_stream *) userData;

    hytera_camera_data_init();

    status = hytera_camera_data_socket_init();
    if (status != PJ_SUCCESS) {

        HYTERA_DEV_LOG("hytera_camera_data_socket_init failed");
        goto on_return;
    }

    status = hytera_camera_data_mediacodec_decoder_init(strm->cam_size.w, strm->cam_size.h, strm->fps_num);
    if (status != PJ_SUCCESS) {

        HYTERA_DEV_LOG("hytera_camera_data_mediacodec_decoder_init failed");
        goto on_return;
    }

    hytera_camera_data_socket_server_recv_handle(strm);

on_return:

    hytera_camera_data_deinit();
    return PJ_SUCCESS;
}

static pj_status_t hytera_camera_data_init() {

    pj_status_t status = PJ_SUCCESS;

    pj_bzero(&camera_data_var, sizeof(hytera_camera_data));

    camera_data_var.server_socket_fd = -1;
    camera_data_var.client_socket_fd = -1;
    camera_data_var.socket_server_quit_flag = PJ_FALSE;

    camera_data_var.dec          = NULL;
    camera_data_var.dec_buf      = NULL;
    camera_data_var.dec_buf_size = 0;

    camera_data_var.rec_whole_frame_buf      = NULL;
    camera_data_var.rec_whole_frame_buf_size = 0;
    camera_data_var.first_h264_frame         = PJ_TRUE;

    return status;
}

static pj_status_t hytera_camera_data_socket_init() {

    pj_status_t status;

    pj_sock_t server_socket_fd = -1, client_socket_fd = -1;
    pj_sockaddr_in serverAddr, clientAddr;

    pj_bzero(&serverAddr, sizeof(serverAddr));
    pj_bzero(&clientAddr, sizeof(clientAddr));

    // create server socket
    status = pj_sock_socket(pj_AF_INET(), pj_SOCK_STREAM(), 0, &server_socket_fd);
    if (status != PJ_SUCCESS) {

        HYTERA_DEV_LOG("server socket created failed!");
        goto on_return;
    }

    camera_data_var.server_socket_fd = server_socket_fd;

    // server socket bind
    serverAddr.sin_addr.s_addr = INADDR_ANY; // 指的是本机地址
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = pj_htons(12306); // 固定端口

    status = pj_sock_bind(server_socket_fd, &serverAddr, sizeof(serverAddr));
    if (status != PJ_SUCCESS) {

        HYTERA_DEV_LOG("server socket bind failed!");
        goto on_return;
    }

    // server socket listen
    status = pj_sock_listen(server_socket_fd, 8);
    if (status != PJ_SUCCESS) {

        HYTERA_DEV_LOG("server socket listen failed!");
        goto on_return;
    }

    // accept
    while (!camera_data_var.socket_server_quit_flag) {

        status = hytera_camera_data_socket_accept(server_socket_fd, &client_socket_fd);
        if (status != PJ_SUCCESS) {
            continue;
        }
        break;
    }

    camera_data_var.client_socket_fd = client_socket_fd;
    if (camera_data_var.socket_server_quit_flag) {

        status = PJ_EGONE;
        goto on_return;
    }

    return PJ_SUCCESS;

on_return:

    hytera_camera_data_socket_deinit();
    return status;
}

static pj_status_t hytera_camera_data_socket_deinit() {

    camera_data_var.socket_server_quit_flag = PJ_TRUE;

    if (camera_data_var.server_socket_fd >= 0) {

        pj_sock_close(camera_data_var.server_socket_fd);
        camera_data_var.server_socket_fd = -1;
    }

    if (camera_data_var.client_socket_fd >= 0) {

        pj_sock_close(camera_data_var.client_socket_fd);
        camera_data_var.client_socket_fd = -1;
    }

    return PJ_SUCCESS;
}

static pj_status_t hytera_camera_data_deinit() {

    camera_data_var.first_h264_frame = PJ_FALSE;

    hytera_camera_data_socket_deinit();
    hytera_camera_data_mediacodec_decoder_deinit();
    return PJ_SUCCESS;
}

/**
 * 海能达设备套接字 accept 超时处理，避免调用 accept 时造成阻塞
 */
static pj_status_t hytera_camera_data_socket_accept(pj_sock_t serverfd, pj_sock_t *clientfd) {

    pj_status_t status = PJ_SUCCESS;
    unsigned int wait_usec = 1000 * 50; // 等待时间 单位为微秒 暂定 50ms轮询一次
    int ret = -1;

    /**
     * 超时等待处理，在 ret 的值 ret <= 0 时不调用 pj_sock_accept 避免阻塞，当 ret > 0 时，
     * 此时已能正常获取得到客户端连接的 clientfd
     */
    if (wait_usec > 0) {

        fd_set accept_fdset;
        struct timeval timeout;
        FD_ZERO(&accept_fdset);
        FD_SET(serverfd, &accept_fdset);

        timeout.tv_sec = 0;
        timeout.tv_usec = wait_usec;

        do {
            ret = select(serverfd + 1, &accept_fdset, NULL, NULL, &timeout);
        } while (ret < 0);

        if (ret == -1) return -1;
        else if (ret == 0) {
            return -1;
        }
    }

    status = pj_sock_accept(serverfd, clientfd, NULL, NULL);
    return status;
}

static void hytera_camera_data_socket_server_recv_handle(hytera_stream *strm) {

    pj_status_t status;

    pj_ssize_t frameBufSize = strm->cam_size.w * strm->cam_size.h * 3 / 2;
    pj_uint8_t  *frameBuf = malloc(frameBufSize);
    if (frameBuf == NULL) {

        HYTERA_DEV_LOG("frameBuf malloc failed");
        return;
    }

    memset(frameBuf, 0, frameBufSize);

    while (!camera_data_var.socket_server_quit_flag) {

        pj_ssize_t bufLen = frameBufSize;

        status = pj_sock_recv(camera_data_var.client_socket_fd, frameBuf, &bufLen, 0);
        if (status == PJ_SUCCESS) {

            if (camera_data_var.first_h264_frame == PJ_TRUE) { // 首次读取的数据为单独的 SPS PPS 信息
                camera_data_var.first_h264_frame = PJ_FALSE;
                continue;
            }

            for (int pos = 0; pos < bufLen; pos++) {

                if (pos + 4 < bufLen) {

                    if (frameBuf[pos] == 0x00
                        && frameBuf[pos + 1] == 0x00
                        && frameBuf[pos + 2] == 0x00
                        && frameBuf[pos + 3] == 0x01) { // 一帧的开始，上一帧的结束 00 00 00 01

                        if (camera_data_var.rec_whole_frame_buf_size > 0) {
                            hytera_camera_data_mediacodec_decode(strm, (char *)camera_data_var.rec_whole_frame_buf, camera_data_var.rec_whole_frame_buf_size);
                            camera_data_var.rec_whole_frame_buf_size = 0;
                        }
                    }
                }

                camera_data_var.rec_whole_frame_buf[camera_data_var.rec_whole_frame_buf_size] = frameBuf[pos];
                camera_data_var.rec_whole_frame_buf_size++;
            }
        } else {
            HYTERA_DEV_LOG("pj_sock_recv data failed, please check it out!");
        }
    }

    if (frameBuf != NULL) {

        free(frameBuf);
        frameBuf = NULL;
    }
}

/**
 * NDK decodec  AMediaCodec
 */
static unsigned long long timeGetTime() {

    struct timeval tv;
    gettimeofday(&tv,NULL);
    return (unsigned long long)(tv.tv_sec)*1000000 + tv.tv_usec;
}

static pj_status_t hytera_camera_data_mediacodec_decoder_init(unsigned width, unsigned height, int fps) {

    const char *mime_type = "video/avc";
    camera_data_var.dec = NULL;

    AMediaCodec *dec = AMediaCodec_createDecoderByType(mime_type);
    if (dec == NULL) {
        PJ_LOG(4, (THIS_FILE, "AMediaCodec_createDecoderByType failed!"));
        return PJ_EINVALIDOP;
    }

    AMediaFormat *format = AMediaFormat_new();
    AMediaFormat_setString(format, AMEDIAFORMAT_KEY_MIME, mime_type);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_WIDTH, width);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_HEIGHT, height);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_MAX_WIDTH, width);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_MAX_HEIGHT, height);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_MAX_INPUT_SIZE, width * height);

    // 配置解码器
    media_status_t status = AMediaCodec_configure(dec, format, NULL, NULL, 0);
    if (status != AMEDIA_OK) {
        PJ_LOG(4, (THIS_FILE, "AMediaCodec_configure dec failed!"));
        goto on_error;
    }

    status = AMediaCodec_start(dec);
    if (status != AMEDIA_OK) {
        PJ_LOG(4, (THIS_FILE, "AMediaCodec_start dec failed!"));
        goto on_error;
    }

    AMediaFormat_delete(format);

    camera_data_var.first_h264_frame = PJ_TRUE;
    camera_data_var.dec_buf_size = width * height * 3 >> 1;
    camera_data_var.dec_buf = malloc(camera_data_var.dec_buf_size);
    if (camera_data_var.dec_buf == NULL) {

        camera_data_var.dec_buf_size = 0;
        goto  on_error;
    }

    camera_data_var.rec_whole_frame_buf_size = 0;
    camera_data_var.rec_whole_frame_buf = malloc(camera_data_var.dec_buf_size);
    if (camera_data_var.rec_whole_frame_buf == NULL) {
        goto on_error;
    }

    camera_data_var.dec = dec;
    PJ_LOG(4, (THIS_FILE, "create dec successed!"));
    return PJ_SUCCESS;

on_error:
    if (dec != NULL) {
        AMediaCodec_delete(dec);
    }

    if (format != NULL) {
        AMediaFormat_delete(format);
    }

    return PJ_EINVALIDOP;
}

static pj_status_t hytera_camera_data_mediacodec_decode(hytera_stream *strm, char *inBuf, int inBufSize) {

    if (camera_data_var.dec == NULL) {
        return PJ_EINVAL;
    }

    /** -1 表示一直等待 0 表示不等待 其他大于0的参数表示等待微秒数 */
    ssize_t ibufidx = AMediaCodec_dequeueInputBuffer(camera_data_var.dec, 0);
    if (ibufidx >= 0) {

        size_t ibufsize; // 获取 inputBuf 索引
        uint8_t *inputBuf = AMediaCodec_getInputBuffer(camera_data_var.dec, ibufidx, &ibufsize);
        if (inputBuf != NULL) {

            if (ibufsize < inBufSize) {
                return PJ_EINVAL;
            }

            pj_memset(inputBuf, 0, inBufSize);
            pj_memcpy(inputBuf, inBuf, inBufSize);

            // 将数据传递进解码器解码
            media_status_t status = AMediaCodec_queueInputBuffer(camera_data_var.dec, ibufidx, 0, inBufSize, timeGetTime(), 0);
        }
    } else {

        if (ibufidx == AMEDIA_ERROR_UNKNOWN) {
            return PJ_EINVAL;
        }
    }

    size_t obufsize = 0;
    AMediaCodecBufferInfo bufferInfo;

    // 从输出缓冲区队列拿到解码好的内容，对内容进行相应处理后再释放
    ssize_t obufidx = AMediaCodec_dequeueOutputBuffer(camera_data_var.dec, &bufferInfo, 0);
    while (obufidx >= 0) {

        uint8_t *outputBuf = AMediaCodec_getOutputBuffer(camera_data_var.dec, obufidx, &obufsize);
        if (outputBuf == NULL) {
            return PJ_EINVAL;
        }

        if (bufferInfo.size > camera_data_var.dec_buf_size) {
            PJ_LOG(5, (THIS_FILE, "Decode Failed, bufferInfo.size:[%d] < dec_buf_size:[%d]",
                    bufferInfo.size, camera_data_var.dec_buf_size));
            return PJ_EINVAL;
        }

        pj_memcpy(camera_data_var.dec_buf, outputBuf, bufferInfo.size);

        size_t u_size = bufferInfo.size / 6;
        size_t y_size = u_size << 2;

        // YUV 数据转换 编码出来的 colorformat 为 21(NV12) 需要转成 I420
        for (int i = 0; i < u_size; i++) { // NV12 转 I420

            camera_data_var.dec_buf[y_size + i] = outputBuf[y_size + 2 * i];
            camera_data_var.dec_buf[y_size + u_size + i] = outputBuf[y_size + 2 * i + 1];
        }

        strm->frame_ts.u64 += strm->ts_inc;

        // 数据处理
        pjmedia_frame f;
        f.type = PJMEDIA_FRAME_TYPE_VIDEO;
        f.size = bufferInfo.size;
        f.timestamp.u64 = strm->frame_ts.u64;
        f.buf = camera_data_var.dec_buf;

        (*strm->vid_cb.capture_cb)(&strm->base, strm->user_data, &f);

        AMediaCodec_releaseOutputBuffer(camera_data_var.dec, obufidx, false);
        obufidx = AMediaCodec_dequeueOutputBuffer(camera_data_var.dec, &bufferInfo, 0);
    }

    return PJ_SUCCESS;
}

static pj_status_t hytera_camera_data_mediacodec_decoder_deinit() {

    if (camera_data_var.dec != NULL) {

        AMediaCodec_stop(camera_data_var.dec);
        AMediaCodec_delete(camera_data_var.dec);
        camera_data_var.dec = NULL;
    }

    if (camera_data_var.dec_buf != NULL) {

        free(camera_data_var.dec_buf);
        camera_data_var.dec = NULL;
    }

    if (camera_data_var.rec_whole_frame_buf != NULL) {

        free(camera_data_var.rec_whole_frame_buf);
        camera_data_var.rec_whole_frame_buf = NULL;
    }
}


#endif














