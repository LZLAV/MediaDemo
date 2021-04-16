cmake_minimum_required(VERSION 3.4.1)

message(WARNING "########make pjmedia-videodev-lib ################")

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

add_library( pjmedia-videodev-lib STATIC 
		../../src/pjmedia-videodev/android_dev.c
		../../src/pjmedia-videodev/android_opengl.c
		../../src/pjmedia-videodev/sdl_dev.c
		../../src/pjmedia-videodev/videodev.c
		../../src/pjmedia-videodev/errno.c
		../../src/pjmedia-videodev/avi_dev.c
		../../src/pjmedia-videodev/ffmpeg_dev.c
		../../src/pjmedia-videodev/colorbar_dev.c
		../../src/pjmedia-videodev/v4l2_dev.c
		../../src/pjmedia-videodev/opengl_dev.c
		../../src/pjmedia-videodev/util.c
		../../src/pjmedia-videodev/vid_save.c
		)

find_library(log-lib log)


target_link_libraries(

		pjmedia-videodev-lib
		log
		)

