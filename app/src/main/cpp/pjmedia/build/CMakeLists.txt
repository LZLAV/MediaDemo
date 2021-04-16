cmake_minimum_required(VERSION 3.4.1)

message(WARNING "########make pjmedia-lib ################")

add_definitions( -DPJ_AUTOCONF=1 )
add_definitions( -DNDEBUG )
add_definitions( -DANDROID )
add_definitions( -DPJ_IS_BIG_ENDIAN=0 )
add_definitions( -DPJ_IS_LITTLE_ENDIAN=1 )

include_directories(
		../../auto_config/${ANDROID_ABI}
                    include
                    ../../pjlib/include
					../../pjlib-util/include
					../../pjnath/include
					../../pjmedia/include
                    ../../
                   )

add_library( pjmedia-lib STATIC 
		../src/pjmedia/echo_speex.c
		../src/pjmedia/alaw_ulaw.c
		../src/pjmedia/alaw_ulaw_table.c
		../src/pjmedia/avi_player.c
		../src/pjmedia/bidirectional.c
		../src/pjmedia/clock_thread.c
		../src/pjmedia/codec.c
		../src/pjmedia/conference.c
		../src/pjmedia/conf_switch.c
		../src/pjmedia/converter.c
		../src/pjmedia/converter_libswscale.c
		../src/pjmedia/converter_libyuv.c
		../src/pjmedia/delaybuf.c
		../src/pjmedia/echo_common.c
		../src/pjmedia/echo_port.c
		../src/pjmedia/echo_suppress.c
		../src/pjmedia/echo_webrtc.c
		../src/pjmedia/endpoint.c
		../src/pjmedia/errno.c
		../src/pjmedia/event.c
		../src/pjmedia/format.c
		../src/pjmedia/ffmpeg_util.c
		../src/pjmedia/g711.c
		../src/pjmedia/jbuf.c
		../src/pjmedia/master_port.c
		../src/pjmedia/mem_capture.c
		../src/pjmedia/mem_player.c
		../src/pjmedia/null_port.c
		../src/pjmedia/plc_common.c
		../src/pjmedia/port.c
		../src/pjmedia/splitcomb.c
		../src/pjmedia/resample_resample.c
		../src/pjmedia/resample_libsamplerate.c
		../src/pjmedia/resample_speex.c
		../src/pjmedia/resample_port.c
		../src/pjmedia/rtcp.c
		../src/pjmedia/rtcp_xr.c
		../src/pjmedia/rtcp_fb.c
		../src/pjmedia/rtp.c
		../src/pjmedia/sdp.c
		../src/pjmedia/sdp_cmp.c
		../src/pjmedia/sdp_neg.c
		../src/pjmedia/session.c
		../src/pjmedia/silencedet.c
		../src/pjmedia/sound_legacy.c
		../src/pjmedia/sound_port.c
		../src/pjmedia/stereo_port.c
		../src/pjmedia/stream_common.c
		../src/pjmedia/stream.c
		../src/pjmedia/stream_info.c
		../src/pjmedia/tonegen.c
		../src/pjmedia/transport_adapter_sample.c
		../src/pjmedia/transport_ice.c
		../src/pjmedia/transport_loop.c
		../src/pjmedia/transport_srtp.c
		../src/pjmedia/transport_udp.c
		../src/pjmedia/types.c
		../src/pjmedia/vid_codec.c
		../src/pjmedia/vid_codec_util.c
		../src/pjmedia/vid_port.c
		../src/pjmedia/vid_stream.c
		../src/pjmedia/vid_stream_info.c
		../src/pjmedia/vid_tee.c
		../src/pjmedia/wav_player.c
		../src/pjmedia/wav_playlist.c
		../src/pjmedia/wav_writer.c
		../src/pjmedia/wave.c
		../src/pjmedia/wsola.c
		../src/pjmedia/audiodev.c
		../src/pjmedia/videodev.c
		)

find_library(log-lib log)


target_link_libraries(
		pjmedia-lib
#        webrtc-lib
		log
		)

