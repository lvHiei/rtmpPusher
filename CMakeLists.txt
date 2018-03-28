# Sets the minimum version of CMake required to build the native
# library. You should either keep the default value or only pass a
# value of 3.4.0 or lower.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds it for you.
# Gradle automatically packages shared libraries with your APK.

set(ANDROID_PROJECT 0)

add_definitions(-DNO_CRYPTO)

if(ANDROID_PROJECT)
    add_definitions(-DANDROID_PROJECT)
endif()

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/librtmp)

if(NOT ANDROID_PROJECT)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
endif()

add_library(
    rtmp

    STATIC

    librtmp/amf.c
    librtmp/hashswf.c
    librtmp/log.c
    librtmp/parseurl.c
    librtmp/rtmp.c

)

add_library(
    rtmpPush

    SHARED

    RtmpPusher.cpp
)

if(ANDROID_PROJECT)
    find_library( # Sets the name of the path variable.
                  log-lib

                  # Specifies the name of the NDK library that
                  # you want CMake to locate.
                log )
endif()

if(ANDROID_PROJECT)
    target_link_libraries( rtmpPush
        rtmp
        ${log-lib}
    )
else()
    target_link_libraries( rtmpPush
        rtmp
    )
endif()



