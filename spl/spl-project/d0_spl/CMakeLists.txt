cmake_minimum_required(VERSION 3.15)

include(proj.conf)

find_package(bouffalo_sdk REQUIRED HINTS $ENV{BL_SDK_BASE})

target_sources(app PRIVATE rv32i_xtheade_lz4.S)
sdk_set_main_file(main.c)

project(spl)
