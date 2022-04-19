# CMakeLists.txt
#
# The Great Escape in C
#
# Copyright (c) David Thomas, 2017-2021
#
# vim: sw=4 ts=8 et

add_library(ZXSpectrum
    Kempston.c
    Keyboard.c
    Screen.c
    Spectrum.c
    ../../include/ZXSpectrum/Kempston.h
    ../../include/ZXSpectrum/Keyboard.h
    ../../include/ZXSpectrum/Screen.h
    ../../include/ZXSpectrum/Spectrum.h)

target_include_directories(ZXSpectrum
    PUBLIC
    ../../include/
    PRIVATE
    include/)
