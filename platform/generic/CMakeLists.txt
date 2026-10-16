set(TARGET ${PROJECT_NAME}Headless)

add_executable(${TARGET} main.c)
target_link_libraries(${TARGET} ZXGameHeadless TheGreatEscape)

if(TARGET ZXGameSDL3)
  add_executable(${PROJECT_NAME}SDL3 sdl3main.c)
  target_link_libraries(${PROJECT_NAME}SDL3 ZXGameSDL3 TheGreatEscape)
  zxgame_sdl3_app(${PROJECT_NAME}SDL3)
endif()
