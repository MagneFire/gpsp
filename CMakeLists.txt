cmake_minimum_required(VERSION 3.6.0)

project(gpsp
        VERSION 0.0.1
        DESCRIPTION "gpSP for AsteroidOS")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/asteroidos")

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 sdl2 IMPORTED_TARGET REQUIRED)
set_property(GLOBAL APPEND PROPERTY _CMAKE_sdl2_TYPE REQUIRED)

if(SDL2_FOUND)
	set_property(GLOBAL APPEND PROPERTY PACKAGES_FOUND sdl2)
else()
	set_property(GLOBAL APPEND PROPERTY PACKAGES_NOT_FOUND sdl2)
endif()


find_package(PkgConfig REQUIRED)
pkg_check_modules(HYEGLP hybris-egl-platform IMPORTED_TARGET REQUIRED)
set_property(GLOBAL APPEND PROPERTY _CMAKE_hybris-egl-platform_TYPE REQUIRED)

if(HYEGLP)
	set_property(GLOBAL APPEND PROPERTY PACKAGES_FOUND hybris-egl-platform)
else()
	set_property(GLOBAL APPEND PROPERTY PACKAGES_NOT_FOUND hybris-egl-platform)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(HYEGL hwcomposer-egl IMPORTED_TARGET REQUIRED)
set_property(GLOBAL APPEND PROPERTY _CMAKE_hwcomposer-egl_TYPE REQUIRED)

if(HYEGL)
	set_property(GLOBAL APPEND PROPERTY PACKAGES_FOUND hwcomposer-egl)
else()
	set_property(GLOBAL APPEND PROPERTY PACKAGES_NOT_FOUND hwcomposer-egl)
endif()


find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBHW libhardware IMPORTED_TARGET REQUIRED)
set_property(GLOBAL APPEND PROPERTY _CMAKE_libhardware_TYPE REQUIRED)

if(LIBHW)
	set_property(GLOBAL APPEND PROPERTY PACKAGES_FOUND libhardware)
else()
	set_property(GLOBAL APPEND PROPERTY PACKAGES_NOT_FOUND libhardware)
endif()

set_property(SOURCE arm/arm_stub.S PROPERTY LANGUAGE C)
set_property(SOURCE arm/video_blend.S PROPERTY LANGUAGE C)

set(SOURCES
        asteroidos/asteroid.c
        asteroidos/hwcomposer.cpp
        asteroidos/gles_video.c
        main.c
        memory.c
        video.c
        input.c
        sound.c
        gui.c
        cheats.c
        zip.c
        arm/arm_stub.S
        arm/warm.c
        arm/video_blend.S
        cpu.c
        cpu_threaded.c
        )

set(INCLUDES
        )

add_definitions(-DARM_ARCH -DASTEROID_BUILD -DQCOM_BSP)

add_executable(gpsp ${SOURCES})
target_include_directories(gpsp PUBLIC ${INCLUDES})

target_link_libraries(gpsp PUBLIC
        PkgConfig::HYEGLP
        PkgConfig::HYEGL
        PkgConfig::LIBHW
        PkgConfig::SDL2
        -lz -lGLESv2 -lEGL)
