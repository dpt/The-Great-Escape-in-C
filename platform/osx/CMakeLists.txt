set(TARGET ${PROJECT_NAME})

set(SOURCES
  AppDelegate.m
  ZXGameView.m
  ZXGameWindow.m
  ZXGameWindowController.m
  bitfifo.c
  main.m
)

set(RESOURCES
  AppIcon.icns
# ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.icns
  en.lproj/Credits.rtf
)

set(RESOURCES_SOURCE_DIR
  ${PROJECT_SOURCE_DIR}/platform/osx)

set(XIBS
  en.lproj/MainMenu
  en.lproj/ZXGameWindow
)

set(APPICONSET ${RESOURCES_SOURCE_DIR}/Images.xcassets/AppIcon.appiconset)

add_executable(${TARGET}
  MACOSX_BUNDLE
  ${SOURCES}
  ${RESOURCES}
)

set_target_properties(${TARGET}
  PROPERTIES
  MACOSX_BUNDLE_BUNDLE_NAME "${DESCRIPTION}"
  MACOSX_BUNDLE_BUNDLE_VERSION "${APPLICATION_VERSION}"
  MACOSX_BUNDLE_COPYRIGHT "${COPYRIGHT}"
  MACOSX_BUNDLE_GUI_IDENTIFIER "${CODE_IDENTIFIER}"
  MACOSX_BUNDLE_ICON_FILE "AppIcon.icns"
  MACOSX_BUNDLE_INFO_STRING "${DESCRIPTION}"
  MACOSX_BUNDLE_LONG_VERSION_STRING "${VERSION}"
  MACOSX_BUNDLE_SHORT_VERSION_STRING "${VERSION}"
)

find_library(AUDIOUNIT_LIBRARY AudioUnit)
find_library(COCOA_LIBRARY Cocoa)
find_library(COREIMAGE_LIBRARY CoreImage)
find_library(OPENGL_LIBRARY OpenGL)

target_link_libraries(${TARGET}
  LibZXSpectrum
  LibTheGreatEscape
  ${AUDIOUNIT_LIBRARY}
  ${COCOA_LIBRARY}
  ${COREIMAGE_LIBRARY}
  ${OPENGL_LIBRARY}
)

find_program(ICONUTIL_EXE NAMES iconutil)
add_custom_command(
    OUTPUT ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.icns
    COMMAND ${ICONUTIL_EXE} --convert icns --output ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.icns ${APPICONSET}
    DEPENDS ${APPICONSET}
    COMMENT "Building app icon..."
)

# Make sure the 'Resources' directory is correctly created before we build
add_custom_command(TARGET ${TARGET}
  PRE_BUILD
  COMMAND mkdir -p ./\${CONFIGURATION}/${TARGET}.app/Contents/Resources
)

# Place resource files in Resources/*
set_source_files_properties(${RESOURCES}
  PROPERTIES
  MACOSX_PACKAGE_LOCATION Resources
)

# https://cmake.org/Wiki/CMake:OSX_InterfaceBuilderFiles

# Make sure we can find the 'ibtool' program. If we can NOT find it we skip
# generation of this project
find_program(IBTOOL ibtool)
if (${IBTOOL} STREQUAL "IBTOOL-NOTFOUND")
  message(SEND_ERROR "ibtool can not be found and is needed to compile the .xib files.")
endif()

# Compile the .xib files using the 'ibtool' program with the destination
# being the app package
foreach(xib ${XIBS})
  add_custom_command(TARGET ${TARGET}
    POST_BUILD
    COMMAND ${IBTOOL}
      --errors --warnings --notices
      --output-format human-readable-text
      --compile ./\${CONFIGURATION}/${TARGET}.app/Contents/Resources/${xib}.nib ${RESOURCES_SOURCE_DIR}/${xib}.xib
    COMMENT "Compiling ${xib}.xib"
  )
endforeach()
