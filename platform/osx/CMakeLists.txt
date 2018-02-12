set(TARGET TheGreatEscape)

set(SOURCES
  AppDelegate.m
  ZXGameView.m
  ZXGameWindow.m
  ZXGameWindowController.m
  bitfifo.c
  main.m
)

set(XIBS
  en.lproj/MainMenu
  en.lproj/ZXGameWindow
)

set(OTHERS  # other resources
  en.lproj/Credits.rtf
)

add_executable(${TARGET}
  MACOSX_BUNDLE
  ${SOURCES}
  ${RESOURCES}
)

# Package the other stuff into Resources/*
set_source_files_properties(${RESOURCES}
  PROPERTIES
  MACOSX_PACKAGE_LOCATION
  Resources
)

set_target_properties(${TARGET}
  PROPERTIES
  MACOSX_BUNDLE_INFO_PLIST
  ${CMAKE_CURRENT_LIST_DIR}/TheGreatEscape-Info.plist
)

find_library(AUDIOUNIT_LIBRARY AudioUnit)
find_library(COCOA_LIBRARY Cocoa)
find_library(OPENGL_LIBRARY OpenGL)

target_link_libraries(${TARGET}
  LibZXSpectrum
  LibTheGreatEscape
  ${AUDIOUNIT_LIBRARY}
  ${COCOA_LIBRARY}
  ${OPENGL_LIBRARY}
)

# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework AppKit")


set(PLATDIR ${PROJECT_SOURCE_DIR}/platform/osx)

# https://cmake.org/Wiki/CMake:OSX_InterfaceBuilderFiles

# Make sure we can find the 'ibtool' program. If we can NOT find it we skip generation of this project
find_program(IBTOOL ibtool)
if (${IBTOOL} STREQUAL "IBTOOL-NOTFOUND")
  message(SEND_ERROR "ibtool can not be found and is needed to compile the .xib files.")
endif()

# Make sure the 'Resources' Directory is correctly created before we build
add_custom_command(TARGET ${TARGET}
  PRE_BUILD
  COMMAND mkdir -p ./\${CONFIGURATION}/${TARGET}.app/Contents/Resources
)

# Compile the .xib files using the 'ibtool' program with the destination being the app package
foreach(xib ${XIBS})
  add_custom_command(TARGET ${TARGET}
    POST_BUILD
    COMMAND ${IBTOOL}
      --errors --warnings --notices
      --output-format human-readable-text
      --compile ./\${CONFIGURATION}/${TARGET}.app/Contents/Resources/${xib}.nib ${PLATDIR}/${xib}.xib
    COMMENT "Compiling ${xib}.xib"
  )
endforeach()
