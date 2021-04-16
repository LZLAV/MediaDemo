#ifndef __webrtc_apm_H__
#define __webrtc_apm_H__

#ifdef __cplusplus
extern "C" {
#endif


typedef struct webrtc_apm_aec_options
{
    //union
    //Speech Quality
    unsigned char highPassFilter:1;
    unsigned char speechIntelligibilityEnhance:1;
    unsigned char beamForming:1;

    //AEC
    unsigned char aecPCEnable:1;
    unsigned char aecExtendFilter:1;
    unsigned char delayAgnostic:1;
    unsigned char nextGenerationAEC:1;
    unsigned char aecPCLevel:2;         //[0,1,2]

    //NS
    unsigned char nsEnable:1;
    unsigned char experimentalNS:1;
    unsigned char nsLevel:2;            //[0,1,2,3]

    //howling
    unsigned char howlingEnable:1;

} webrtc_apm_aec_options;

typedef struct webrtc_apm_agc_options
{
    unsigned char  agcEnable:1;
    unsigned char  experimentalAGC:1;
    unsigned char   agcMode:2;          //[0,1,2]
    unsigned char   agcTargetLevel;     //[0-31]
    unsigned char   agcCompressionGain; //[0-90]

} webrtc_apm_agc_options;


void webrtc_apm_get_default_aec_options(webrtc_apm_aec_options * opts);
void webrtc_apm_get_default_agc_options(webrtc_apm_agc_options * opts);
void * webrtc_apm_create_instance(const webrtc_apm_aec_options * aec, const webrtc_apm_agc_options * agc);
void webrtc_apm_release_instance(void * apmInst);

void webrtc_apm_process_capture_stream(void * apmInst, short * frm_buf, int offset);
void webrtc_apm_process_playback_stream(void * apmInst, short * frm_buf, int offset);

void webrtc_apm_set_stream_delay(void * apmInst, int delay_ms);

void webrtc_apm_set_stream_analog_level(void * apmInst, int level);
int webrtc_apm_stream_analog_level(void * apmInst);


#ifdef __cplusplus
}
#endif

#endif
