cmake_minimum_required(VERSION 3.4.1)

message(WARNING "########make pjmedia-audiodev-lib ################")

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

add_library( pjmedia-audiodev-lib STATIC 
		../../src/pjmedia-audiodev/audiodev.c
		../../src/pjmedia-audiodev/audiotest.c
		../../src/pjmedia-audiodev/errno.c
		../../src/pjmedia-audiodev/legacy_dev.c
		../../src/pjmedia-audiodev/null_dev.c
		../../src/pjmedia-audiodev/pa_dev.c
		../../src/pjmedia-audiodev/wmme_dev.c
		../../src/pjmedia-audiodev/alsa_dev.c
		../../src/pjmedia-audiodev/bb10_dev.c
		../../src/pjmedia-audiodev/bdimad_dev.c
		../../src/pjmedia-audiodev/android_jni_dev.c
		../../src/pjmedia-audiodev/opensl_dev.c
		)

find_library(log-lib log)


target_link_libraries(
		pjmedia-audiodev-lib
		log
		)

