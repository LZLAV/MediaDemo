cmake_minimum_required(VERSION 3.4.1)

message(WARNING "########make pjmedia-codec-lib ################")

add_definitions( -DPJ_AUTOCONF=1 )
add_definitions( -DNDEBUG )
add_definitions( -DANDROID )
add_definitions( -DPJ_IS_BIG_ENDIAN=0 )
add_definitions( -DPJ_IS_LITTLE_ENDIAN=1 )

include_directories(
		../../../auto_config/${ANDROID_ABI}
                    ../../include
                    ../../../pjlib/include
                    ../../../pjlib-util/include
                    ../../../pjmedia/include
                    ../../..
                   )

add_library( pjmedia-codec-lib STATIC 
		../../src/pjmedia-codec/audio_codecs.c
		../../src/pjmedia-codec/ffmpeg_vid_codecs.c
		../../src/pjmedia-codec/vid_mediacodec.c
		../../src/pjmedia-codec/openh264.cpp
		../../src/pjmedia-codec/h263_packetizer.c
		../../src/pjmedia-codec/h264_packetizer.c
		../../src/pjmedia-codec/ipp_codecs.c
		../../src/pjmedia-codec/silk.c
		../../src/pjmedia-codec/opus.c
		../../src/pjmedia-codec/l16.c
		../../src/pjmedia-codec/gsm.c
		../../src/pjmedia-codec/speex_codec.c
		../../src/pjmedia-codec/ilbc.c
		../../src/pjmedia-codec/g722.c
		../../src/pjmedia-codec/g722/g722_enc.c
		../../src/pjmedia-codec/g722/g722_dec.c
		../../src/pjmedia-codec/g7221.c
		../../src/pjmedia-codec/g7221_sdp_match.c
		../../src/pjmedia-codec/amr_sdp_match.c
		../../src/pjmedia-codec/opencore_amr.c
        )

find_library(log-lib
		log)


target_link_libraries(
		pjmedia-codec-lib
		log
		)

