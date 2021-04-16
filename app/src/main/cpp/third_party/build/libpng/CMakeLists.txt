cmake_minimum_required(VERSION 3.4.1)

project(libpng C)

enable_language(ASM)
add_definitions(-DPNG_ARM_NEON_OPT=0)

include_directories(
        ../../libpng/zlib
)

# OUR SOURCES
set(libpng_public_hdrs
        ../../libpng/png.h
        ../../libpng/pngconf.h
        ../../libpng/pnglibconf.h
        )
set(libpng_private_hdrs
        ../../libpng/pngpriv.h
        ../../libpng/pngdebug.h
        ../../libpng/pnginfo.h
        ../../libpng/pngstruct.h
        )

set(libpng_sources
        ${libpng_public_hdrs}
        ${libpng_private_hdrs}
        ../../libpng/png.c
        ../../libpng/pngerror.c
        ../../libpng/pngget.c
        ../../libpng/pngmem.c
        ../../libpng/pngpread.c
        ../../libpng/pngread.c
        ../../libpng/pngrio.c
        ../../libpng/pngrtran.c
        ../../libpng/pngrutil.c
        ../../libpng/pngset.c
        ../../libpng/pngtrans.c
        ../../libpng/pngwio.c
        ../../libpng/pngwrite.c
        ../../libpng/pngwtran.c
        ../../libpng/pngwutil.c
        ../../libpng/pngtest.c
        # arm
        ../../libpng/arm_init.c
        ../../libpng/filter_neon.S
        ../../libpng/filter_neon_intrinsics.c
        )

add_definitions(-DPNG_DEBUG)

add_library(png STATIC ${libpng_sources} ${libpng_public_hdrs} ${libpng_private_hdrs})
target_link_libraries(png zlib)