set(TARGET ${PROJECT_NAME})

add_executable(${TARGET}
  main.c
)

target_link_libraries(${TARGET}
  LibZXSpectrum
  LibTheGreatEscape
)
