cmake_minimum_required(VERSION 3.5)

option(CROSS_BUILD_ARM "CROSS_BUILD_ARM" ON)

if(CROSS_BUILD_ARM)
    # 配置 ARM 交叉编译
    set(CMAKE_SYSTEM_NAME Linux)    #设置目标系统名字
    set(CMAKE_SYSTEM_PROCESSOR aarch64) #设置目标处理器架构

    # 指定交叉编译器 arm-gcc 和 arm-g++
    set(CMAKE_C_COMPILER           /usr/bin/aarch64-linux-gnu-gcc-12)
    set(CMAKE_CXX_COMPILER         /usr/bin/aarch64-linux-gnu-g++-12)

    # 为编译器添加编译选项
    set(CMAKE_C_FLAGS "-march=armv8-a -mcpu=cortex-a53")
    set(CMAKE_CXX_FLAGS "-march=armv8-a -mcpu=cortex-a53")
endif ()

project(minimodem)
set(PKG_CONFIG_PATH ${PKG_CONFIG_PATH}:${CMAKE_SYSROOT}/usr/lib)
find_package(PkgConfig REQUIRED)
add_subdirectory(src)