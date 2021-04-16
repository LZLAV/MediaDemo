#define PJ_CONFIG_ANDROID 1
#define PJMEDIA_HAS_LEGACY_SOUND_API 0
#define PJMEDIA_HAS_ILBC_CODEC 0
#define PJMEDIA_HAS_G722_CODEC 0
#define PJMEDIA_HAS_G7221_CODEC 0
#define PJMEDIA_HAS_OPUS_CODEC 1
#define PJMEDIA_HAS_G711_CODEC 1
#define PJMEDIA_HAS_GSM_CODEC 0
#define PJMEDIA_HAS_SPEEX_CODEC 0
#define PJMEDIA_HAS_OPENCORE_AMRNB_CODEC 0      //AMR-nb 编解码
#define PJMEDIA_HAS_SILK_CODEC 0           //silk 编解码
#define PJMEDIA_STREAM_ENABLE_KA 1
#define PJMEDIA_HAS_SPEEX_AEC 0
#define PJMEDIA_HAS_WEBRTC_AEC 0
#define PJMEDIA_HAS_APM_AEC 1

#define TRACE_JB            0    /* Enable/disable trace.    */

#define PJMEDIA_HAS_SRTP 0

#define ARM

#define PJMEDIA_SOUND_BUFFER_COUNT        16

#include <pj/config_site_sample.h>

#ifdef PJMEDIA_HAS_L16_CODEC
#  undef PJMEDIA_HAS_L16_CODEC
# endif
#define PJMEDIA_HAS_L16_CODEC 0
#define PJMEDIA_CODEC_L16_HAS_16KHZ_MONO 0

#define PJMEDIA_HAS_VIDEO 1

#define PJMEDIA_HAS_VID_ADAPTER_CODECS VIDADAPTER_CODEC
#define PJMEDIA_HAS_OPENH264_CODEC     OPENH264_CODEC
#define PJMEDIA_HAS_FFMPEG             FFMPEG_CODEC
#define PJMEDIA_VIDEO_DEV_HAS_FFMPEG   PJMEDIA_HAS_FFMPEG

#define PJMEDIA_HAS_VID_MEDIACODEC_CODEC 0
#define PJMEDIA_AUDIO_DEV_HAS_ANDROID_JNI 1
#define PJMEDIA_AUDIO_DEV_HAS_OPENSL 0
#define PJSIP_AUTH_AUTO_SEND_NEXT 0
#define PJMEDIA_ADVERTISE_RTCP 0
#define PJMEDIA_ADD_BANDWIDTH_TIAS_IN_SDP 0
#define PJMEDIA_ADD_RTPMAP_FOR_STATIC_PT 0

#define PJSUA_MAX_CALLS            32

#define PJMEDIA_CONF_USE_AGC 0

#define NDK_MUXER_MP4_ENABLE 1