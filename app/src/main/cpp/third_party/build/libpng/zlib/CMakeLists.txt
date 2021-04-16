cmake_minimum_required(VERSION 3.4.1)

project(zlib C)

set(VERSION "1.2.11")

add_definitions( -DPJ_AUTOCONF=1 )
add_definitions( -DNDEBUG )
add_definitions( -DANDROID )

set(ZLIB_PUBLIC_HDRS
        ../../../libpng/zlib/zconf.h
        ../../../libpng/zlib/zlib.h
)
set(ZLIB_PRIVATE_HDRS
        ../../../libpng/zlib/crc32.h
        ../../../libpng/zlib/deflate.h
        ../../../libpng/zlib/gzguts.h
        ../../../libpng/zlib/inffast.h
        ../../../libpng/zlib/inffixed.h
        ../../../libpng/zlib/inflate.h
        ../../../libpng/zlib/inftrees.h
        ../../../libpng/zlib/trees.h
        ../../../libpng/zlib/zutil.h
)
set(ZLIB_SRCS
        ../../../libpng/zlib/adler32.c
        ../../../libpng/zlib/crc32.c
        ../../../libpng/zlib/compress.c
        ../../../libpng/zlib/deflate.c
        ../../../libpng/zlib/gzclose.c
        ../../../libpng/zlib/gzlib.c
        ../../../libpng/zlib/gzread.c
        ../../../libpng/zlib/gzwrite.c
        ../../../libpng/zlib/inflate.c
        ../../../libpng/zlib/infback.c
        ../../../libpng/zlib/inftrees.c
        ../../../libpng/zlib/inffast.c
        ../../../libpng/zlib/trees.c
        ../../../libpng/zlib/uncompr.c
        ../../../libpng/zlib/zutil.c
)

add_library(zlib STATIC ${ZLIB_SRCS} ${ZLIB_PUBLIC_HDRS} ${ZLIB_PRIVATE_HDRS})