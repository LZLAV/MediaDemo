#include "webrtc_apm.h"
#include <jni.h>

#include "webrtc/modules/audio_processing/include/audio_processing.h"
#include "webrtc/modules/include/module_common_types.h"
#include "webrtc/common_audio/channel_buffer.h"
#include "webrtc/common_audio/include/audio_util.h"
#include "webrtc/common.h"
#include "webrtc/common_audio/resampler/include/resampler.h"

//#define USE_HOWLING_SUPPRESSION

#ifdef USE_HOWLING_SUPPRESSION
#include "webrtc/howling/howling_core.h"
#endif

#include <android/log.h>
#define TAG "APM"
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, TAG, __VA_ARGS__)


using namespace std;
using namespace webrtc;

class ApmCWrapper{
    const int sample_rate_hz = AudioProcessing::kSampleRate16kHz;
    const int num_input_channels = 1;

    const int reverse_sample_rate_hz = AudioProcessing::kSampleRate16kHz;
    const int num_reverse_channels = 1;

public:
    ApmCWrapper(bool aecExtendFilter, bool speechIntelligibilityEnhance, bool delayAgnostic, bool beamforming, bool nextGenerationAec, bool experimentalNs, bool experimentalAgc){

        _beamForming = beamforming;

        Config config;
        config.Set<ExtendedFilter>(new ExtendedFilter(aecExtendFilter));
        config.Set<Intelligibility>(new Intelligibility(speechIntelligibilityEnhance));
        config.Set<DelayAgnostic>(new DelayAgnostic(delayAgnostic));
        config.Set<NextGenerationAec>(new NextGenerationAec(nextGenerationAec));
        config.Set<ExperimentalNs>(new ExperimentalNs(experimentalNs));
        config.Set<ExperimentalAgc>(new ExperimentalAgc(experimentalAgc));

        _apm.reset(AudioProcessing::Create(config));

        _frame = new AudioFrame();
        _reverseFrame = new AudioFrame();

        SetContainerFormat(sample_rate_hz, num_input_channels, _frame, &_float_cb);

        SetContainerFormat(reverse_sample_rate_hz, num_reverse_channels, _reverseFrame,
                           &_revfloat_cb);

        _apm->Initialize({{{_frame->sample_rate_hz_, _frame->num_channels_},
                                  {_frame->sample_rate_hz_, _frame->num_channels_},
                                  {_reverseFrame->sample_rate_hz_, _reverseFrame->num_channels_},
                                  {_reverseFrame->sample_rate_hz_, _reverseFrame->num_channels_}}});

#ifdef USE_HOWLING_SUPPRESSION
        howling_init(&m_howling_state);
        howling_init(&m_reverse_howling_state);
#endif
    }

    ~ApmCWrapper(){
        delete _frame;
        delete _reverseFrame;
#ifdef USE_HOWLING_SUPPRESSION
        howling_free(&m_howling_state);
        howling_free(&m_reverse_howling_state);
#endif
    }

    int ProcessStream(int16_t* data){
#ifdef USE_HOWLING_SUPPRESSION
        int16_t tmpdata[1440];
        howling_suppression(&m_howling_state, data, _frame->samples_per_channel_, tmpdata);
        std::copy(tmpdata, tmpdata + _frame->samples_per_channel_, _frame->data_);
#else
        std::copy(data, data + _frame->samples_per_channel_, _frame->data_);
#endif
        int ret = _apm->ProcessStream(_frame);
        std::copy(_frame->data_, _frame->data_ + _frame->samples_per_channel_, data);
        return ret;
    }

    int ProcessReverseStream(int16_t* data){
        std::copy(data, data + _reverseFrame->samples_per_channel_, _reverseFrame->data_);
        int ret = _apm->ProcessReverseStream(_reverseFrame);
        if(_beamForming){
            std::copy(_reverseFrame->data_, _reverseFrame->data_ + _reverseFrame->samples_per_channel_, data);
        } else{
#ifdef USE_HOWLING_SUPPRESSION
            //int16_t tmpdata[1440];
            //howling_suppression(&m_reverse_howling_state, data, _reverseFrame->samples_per_channel_, tmpdata);
            //std::copy(tmpdata, tmpdata + _reverseFrame->samples_per_channel_, data);
#endif
        }
        return ret;
    }

public:
    unique_ptr<AudioProcessing> _apm;

private:
    template <typename T>
    void SetContainerFormat(int sample_rate_hz,
                            size_t num_channels,
                            AudioFrame* frame,
                            unique_ptr<ChannelBuffer<T> >* cb) {
        SetFrameSampleRate(frame, sample_rate_hz);
        frame->num_channels_ = num_channels;
        cb->reset(new ChannelBuffer<T>(frame->samples_per_channel_, num_channels));
    }

    void SetFrameSampleRate(AudioFrame* frame,
                            int sample_rate_hz) {
        frame->sample_rate_hz_ = sample_rate_hz;
        frame->samples_per_channel_ = AudioProcessing::kChunkSizeMs *
                                      sample_rate_hz / 1000;
    }

    void ConvertToFloat(const int16_t* int_data, ChannelBuffer<float>* cb) {
        ChannelBuffer<int16_t> cb_int(cb->num_frames(),
                                      cb->num_channels());
        Deinterleave(int_data,
                     cb->num_frames(),
                     cb->num_channels(),
                     cb_int.channels());
        for (size_t i = 0; i < cb->num_channels(); ++i) {
            S16ToFloat(cb_int.channels()[i],
                       cb->num_frames(),
                       cb->channels()[i]);
        }
    }

    void ConvertToFloat(const AudioFrame& frame, ChannelBuffer<float>* cb) {
        ConvertToFloat(frame.data_, cb);
    }

private:
    AudioFrame *_frame;
    AudioFrame *_reverseFrame;

    unique_ptr<ChannelBuffer<float>> _float_cb;
    unique_ptr<ChannelBuffer<float>> _revfloat_cb;

    bool _beamForming = false;

#ifdef USE_HOWLING_SUPPRESSION
    howling_state_t m_howling_state;
    howling_state_t m_reverse_howling_state;
#endif
};



// interface imp

#ifdef __cplusplus
extern "C" {
#endif

static webrtc_apm_aec_options * s_aec_options = NULL;
static webrtc_apm_agc_options * s_agc_options = NULL;

static void init_apm_options()
{
    if(!s_aec_options){
        s_aec_options = new webrtc_apm_aec_options;

        s_aec_options->highPassFilter = 1;
        s_aec_options->speechIntelligibilityEnhance = 1;
        s_aec_options->beamForming = 0;

        s_aec_options->aecPCEnable = 1;
        s_aec_options->aecExtendFilter = 0;
        s_aec_options->delayAgnostic = 1;
        s_aec_options->nextGenerationAEC = 0;
        s_aec_options->aecPCLevel = 1;

        s_aec_options->nsEnable = 1;
        s_aec_options->experimentalNS = 1;
        s_aec_options->nsLevel = 1;
    }

    if(!s_agc_options){
        s_agc_options = new webrtc_apm_agc_options;
        s_agc_options->agcEnable = 1;
        s_agc_options->experimentalAGC = 0;
        s_agc_options->agcMode = 2;
        s_agc_options->agcTargetLevel = 9;
        s_agc_options->agcCompressionGain = 9;
    }

}


void webrtc_apm_get_default_aec_options(webrtc_apm_aec_options * opts)
{
    if(!opts)
        return;

    init_apm_options();
    memcpy(opts, s_aec_options, sizeof(webrtc_apm_aec_options));

}

void webrtc_apm_get_default_agc_options(webrtc_apm_agc_options * opts)
{
    if(!opts)
        return;

    init_apm_options();
    memcpy(opts, s_agc_options, sizeof(webrtc_apm_agc_options));
}

void * webrtc_apm_create_instance(const webrtc_apm_aec_options * aec_opts, const webrtc_apm_agc_options * agc_opts)
{
    ApmCWrapper* apm = new ApmCWrapper(aec_opts->aecExtendFilter,
                                       aec_opts->speechIntelligibilityEnhance,
                                       aec_opts->delayAgnostic,
                                       aec_opts->beamForming,
                                       aec_opts->nextGenerationAEC,
                                       aec_opts->experimentalNS,
                                       agc_opts->experimentalAGC);


    apm->_apm->high_pass_filter()->Enable(aec_opts->highPassFilter);

    apm->_apm->echo_cancellation()->enable_drift_compensation(false);
    apm->_apm->echo_cancellation()->Enable(aec_opts->aecPCEnable);
    apm->_apm->echo_cancellation()->set_suppression_level((EchoCancellation::SuppressionLevel)aec_opts->aecPCLevel);

    apm->_apm->noise_suppression()->Enable(aec_opts->nsEnable);
    apm->_apm->noise_suppression()->set_level((NoiseSuppression::Level)(aec_opts->nsLevel));

    if(agc_opts->agcEnable){
        apm->_apm->gain_control()->set_analog_level_limits(0, 255);
        apm->_apm->gain_control()->set_mode((GainControl::Mode)agc_opts->agcMode);
        apm->_apm->gain_control()->set_target_level_dbfs(agc_opts->agcTargetLevel);
        apm->_apm->gain_control()->set_compression_gain_db(agc_opts->agcCompressionGain);
        apm->_apm->gain_control()->enable_limiter(true);
        apm->_apm->gain_control()->Enable(true);
    }

    return apm;
}

void webrtc_apm_release_instance(void * apmInst)
{
    ApmCWrapper* apm = (ApmCWrapper*)apmInst;
    delete apm;
}

void webrtc_apm_process_capture_stream(void * apmInst, short * frm_buf, int offset)
{
    ApmCWrapper* apm = (ApmCWrapper*)apmInst;
    if(!apmInst)
        return;
    apm->ProcessStream(frm_buf + offset);
}

void webrtc_apm_process_playback_stream(void * apmInst, short * frm_buf, int offset)
{
    ApmCWrapper* apm = (ApmCWrapper*)apmInst;
    if(!apmInst)
        return;
    apm->ProcessReverseStream(frm_buf + offset);
}

void webrtc_apm_set_stream_delay(void * apmInst, int delay_ms)
{
    ApmCWrapper* apm = (ApmCWrapper*)apmInst;
    if(!apmInst)
        return;

    apm->_apm->set_stream_delay_ms(delay_ms);
}

void webrtc_apm_set_stream_analog_level(void * apmInst, int level)
{
    ApmCWrapper* apm = (ApmCWrapper*)apmInst;
    if(!apmInst)
        return;

    apm->_apm->gain_control()->set_stream_analog_level(level);
}

int webrtc_apm_stream_analog_level(void * apmInst)
{
    ApmCWrapper* apm = (ApmCWrapper*)apmInst;
    if(!apmInst)
        return 200;

    return apm->_apm->gain_control()->stream_analog_level();
}



// jni

static void set_ctx(JNIEnv *env, jobject thiz, void *ctx) {
    jclass cls = env->GetObjectClass(thiz);
    jfieldID fid = env->GetFieldID(cls, "objData", "J");
    env->SetLongField(thiz, fid, (jlong)ctx);
}

static void *get_ctx(JNIEnv *env, jobject thiz) {
    jclass cls = env->GetObjectClass(thiz);
    jfieldID fid = env->GetFieldID(cls, "objData", "J");
    return (void*)env->GetLongField(thiz, fid);
}

/*
JNIEXPORT jint JNI_OnLoad(JavaVM *vm , void *reserved ) {
return JNI_VERSION_1_6;
}
*/

#define DEF_JNI_INTERFACE_SET_ENABLE(opt, key) JNIEXPORT jint JNICALL Java_com_csipsdk_sdk_sound_ApmOption_set_1##key \
        (JNIEnv *env, jobject thiz, jboolean enable) { \
            init_apm_options();  \
            opt->key = enable;    \
            return 0;   \
        }

#define DEF_JNI_INTERFACE_SET_LEVEL(opt, key) JNIEXPORT jint JNICALL Java_com_csipsdk_sdk_sound_ApmOption_set_1##key \
        (JNIEnv *env, jobject thiz, jint level){    \
            init_apm_options(); \
            opt->key = level; \
            return 0;   \
        }

#define DEF_JNI_INTERFACE_GET_ENABLE(opt, key) JNIEXPORT jboolean JNICALL Java_com_csipsdk_sdk_sound_ApmOption_get_1##key \
        (JNIEnv *env, jobject thiz) { \
            init_apm_options();  \
            return opt->key != 0;    \
        }

#define DEF_JNI_INTERFACE_GET_LEVEL(opt, key) JNIEXPORT jboolean JNICALL Java_com_csipsdk_sdk_sound_ApmOption_get_1##key \
        (JNIEnv *env, jobject thiz) { \
            init_apm_options();  \
            return opt->key;    \
        }

DEF_JNI_INTERFACE_SET_ENABLE(s_aec_options, highPassFilter)
DEF_JNI_INTERFACE_SET_ENABLE(s_aec_options, speechIntelligibilityEnhance)
DEF_JNI_INTERFACE_SET_ENABLE(s_aec_options, beamForming)

DEF_JNI_INTERFACE_SET_ENABLE(s_aec_options, aecPCEnable)
DEF_JNI_INTERFACE_SET_ENABLE(s_aec_options, aecExtendFilter)
DEF_JNI_INTERFACE_SET_ENABLE(s_aec_options, delayAgnostic)
DEF_JNI_INTERFACE_SET_ENABLE(s_aec_options, nextGenerationAEC)
DEF_JNI_INTERFACE_SET_LEVEL(s_aec_options, aecPCLevel)

DEF_JNI_INTERFACE_SET_ENABLE(s_aec_options, nsEnable)
DEF_JNI_INTERFACE_SET_ENABLE(s_aec_options, experimentalNS)
DEF_JNI_INTERFACE_SET_LEVEL(s_aec_options, nsLevel)


DEF_JNI_INTERFACE_SET_ENABLE(s_agc_options, agcEnable)
DEF_JNI_INTERFACE_SET_ENABLE(s_agc_options, experimentalAGC)
DEF_JNI_INTERFACE_SET_LEVEL(s_agc_options, agcMode)
DEF_JNI_INTERFACE_SET_LEVEL(s_agc_options, agcTargetLevel)
DEF_JNI_INTERFACE_SET_LEVEL(s_agc_options, agcCompressionGain)

//get
DEF_JNI_INTERFACE_GET_ENABLE(s_aec_options, highPassFilter)
DEF_JNI_INTERFACE_GET_ENABLE(s_aec_options, speechIntelligibilityEnhance)
DEF_JNI_INTERFACE_GET_ENABLE(s_aec_options, beamForming)

DEF_JNI_INTERFACE_GET_ENABLE(s_aec_options, aecPCEnable)
DEF_JNI_INTERFACE_GET_ENABLE(s_aec_options, aecExtendFilter)
DEF_JNI_INTERFACE_GET_ENABLE(s_aec_options, delayAgnostic)
DEF_JNI_INTERFACE_GET_ENABLE(s_aec_options, nextGenerationAEC)
DEF_JNI_INTERFACE_GET_LEVEL(s_aec_options, aecPCLevel)

DEF_JNI_INTERFACE_GET_ENABLE(s_aec_options, nsEnable)
DEF_JNI_INTERFACE_GET_ENABLE(s_aec_options, experimentalNS)
DEF_JNI_INTERFACE_GET_LEVEL(s_aec_options, nsLevel)


DEF_JNI_INTERFACE_GET_ENABLE(s_agc_options, agcEnable)
DEF_JNI_INTERFACE_GET_ENABLE(s_agc_options, experimentalAGC)
DEF_JNI_INTERFACE_GET_LEVEL(s_agc_options, agcMode)
DEF_JNI_INTERFACE_GET_LEVEL(s_agc_options, agcTargetLevel)
DEF_JNI_INTERFACE_GET_LEVEL(s_agc_options, agcCompressionGain)




#ifdef __cplusplus
}
#endif