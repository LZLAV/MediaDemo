
cmake_minimum_required(VERSION 3.4.1)

set(PROJECT_ROOT ./../../../)

project("mediademo")

include_directories(
        config_site.h
        auto_config/${ANDROID_ABI}
        pjmedia/include
        pjlib/include
        pjlib-util/include
)

add_library( # Sets the name of the library.
        media
        SHARED
#        pjsua_aud.c
#        pjsua_vid.c
        pjsua_media.c
        native-lib.cpp
        )

find_library( # Sets the name of the path variable.
        log-lib
        log)

target_link_libraries( # Specifies the target library.
        media
        pjmedia-lib
        pjmedia-codec-lib
        pjmedia-videodev-lib
        pjmedia-audiodev-lib
        pjlib-util-lib
        pjlib-lib

        ${log-lib})

add_subdirectory(pjlib/build)
add_subdirectory(pjlib-util/build)

add_subdirectory(pjmedia/build/pjmedia-codec)
add_subdirectory(pjmedia/build/pjmedia-videodev)
add_subdirectory(pjmedia/build/pjmedia-audiodev)
add_subdirectory(pjmedia/build)
#add_subdirectory(third_party/build/webrtc)