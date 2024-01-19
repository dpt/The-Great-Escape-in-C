set(TARGET ${PROJECT_NAME}App)

add_executable(${TARGET}
  main.c
)

target_link_libraries(${TARGET}
  ZXSpectrum
  TheGreatEscape
)
