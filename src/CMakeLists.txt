project(minimodem)

if(WIN32)
    set(USE_SNDIO OFF)
    set(USE_PULSEAUDIO OFF)
    set(USE_ALSA OFF)
    set(USE_SNDFILE OFF)
else ()
    option(USE_SNDIO "USE_SNDIO" OFF)
    option(USE_PULSEAUDIO "USE_PULSEAUDIO" OFF)
    option(USE_ALSA "USE_ALSA" ON)
    option(USE_SNDFILE "USE_SNDFILE" ON)
endif ()

option(USE_BENCHMARKS "USE_BENCHMARKS" ON)

find_package(FFTW3 REQUIRED)

list(APPEND DEPS_LIBS FFTW3::fftw3f)

add_compile_definitions(VERSION="0.24")

if (USE_SNDIO)
    list(APPEND DEPS_LIBS sndio sio_start)
    add_compile_definitions(USE_SNDIO=1)
else ()
    add_compile_definitions(USE_SNDIO=0)
endif ()

if(USE_PULSEAUDIO)
    find_package(pulseaudio)
    list(APPEND DEPS_LIBS ${pulseaudio_LIBRARIES})
    add_compile_definitions(USE_PULSEAUDIO=1)
else ()
    add_compile_definitions(USE_PULSEAUDIO=0)
endif ()

if(USE_ALSA)
    find_package(ALSA REQUIRED)
    list(APPEND DEPS_LIBS ALSA::ALSA)
    add_compile_definitions(USE_ALSA=1)
else ()
    add_compile_definitions(USE_ALSA=0)
endif ()

if(USE_SNDFILE)
    find_package(SndFile REQUIRED)
    list(APPEND DEPS_LIBS SndFile::sndfile)
    add_compile_definitions(USE_SNDFILE=1)
else ()
    add_compile_definitions(USE_SNDFILE=0)
endif ()

if(USE_BENCHMARKS)
    add_compile_definitions(USE_BENCHMARKS=1)
endif ()

if (WIN32)
    list(APPEND DEPS_LIBS ws2_32 winmm)
endif ()

set(BAUDOT_SRC baudot.h baudot.c)

set(FSK_SRC fsk.h fsk.c)

list(APPEND SIMPLEAUDIO_SRC
        simpleaudio.h
        simpleaudio_internal.h
        simple-tone-generator.c
        simpleaudio-pulse.c
        simpleaudio-alsa.c
        simpleaudio-sndio.c
        simpleaudio-benchmark.c
        simpleaudio-sndfile.c
        simpleaudio-win.cpp
        simpleaudio.c)

if(WIN32)
    list(APPEND SIMPLEAUDIO_SRC win_setitimer.h win_setitimer.c)
endif ()

set(UIC_SRC uic_codes.h uic_codes.c)

set(DATABITS_SRC databits.h
        databits_ascii.c
        databits_binary.c
        databits_callerid.c
        databits_baudot.c ${BAUDOT_SRC}
        databits_uic.c ${UIC_SRC})

set(minimodem_SOURCES minimodem.c ${DATABITS_SRC} ${FSK_SRC} ${SIMPLEAUDIO_SRC})

add_executable(${CMAKE_PROJECT_NAME} ${minimodem_SOURCES})
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${DEPS_LIBS})