add_library(LibZXSpectrum
  Kempston.c
  Keyboard.c
  Screen.c
  Spectrum.c
)

target_include_directories(LibZXSpectrum
  PUBLIC
  ../../include/
)
