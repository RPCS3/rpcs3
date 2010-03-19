project(ZeroGSLinux)

# maybe less is possible but i'm writing this on cmake 2.6 :)
cmake_minimum_required(VERSION 2.6)

include_directories(${ZeroGSLinux_SOURCE_DIR}/..)

add_library(ZeroGSLinux callbacks.c Conf.cpp interface.c Linux.cpp support.c)
