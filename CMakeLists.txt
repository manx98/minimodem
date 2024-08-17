cmake_minimum_required(VERSION 3.5)

project(minimodem)
add_subdirectory(third_part/libsndfile)
include_directories(third_part/libsndfile/include)
add_subdirectory(src)