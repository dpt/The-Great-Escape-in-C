# CMakeLists.txt
#
# The Great Escape in C
#
# Copyright (c) David Thomas, 2020-2021
#
# vim: sw=4 ts=8 et

# Note: bitfifo is in DPTLib now.
set(SOURCES
    AppDelegate.h
    AppDelegate.m
    ZXGameView.h
    ZXGameView.m
    ZXGameWindow.h
    ZXGameWindow.m
    ZXGameWindowController.h
    ZXGameWindowController.m
    bitfifo.c
    bitfifo.h
    main.m
    en.lproj/MainMenu.xib
    en.lproj/ZXGameWindow.xib)

set(GENERATED_ICNS
    ${PROJECT_BINARY_DIR}/platform/osx/AppIcon.icns)

set(RESOURCES
    ${GENERATED_ICNS}
    en.lproj/Credits.rtf)

set(RESOURCES_SOURCE_DIR
    ${PROJECT_SOURCE_DIR}/platform/osx)

set(XIBS
    en.lproj/MainMenu
    en.lproj/ZXGameWindow)

add_executable(TheGreatEscapeApp
    MACOSX_BUNDLE
    ${SOURCES}
    ${RESOURCES})

target_compile_definitions(TheGreatEscapeApp PRIVATE GL_SILENCE_DEPRECATION)

if(CMAKE_GENERATOR STREQUAL "Xcode")
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES CACHE INTERNAL "Enable Objective-C ARC")
endif()

if(TGE_SAVES)
    target_compile_definitions(TheGreatEscapeApp PRIVATE TGE_SAVES)
endif()

set_target_properties(TheGreatEscapeApp
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "$<CONFIG>"
    OUTPUT_NAME "TheGreatEscape"
    MACOSX_BUNDLE_BUNDLE_NAME "${DESCRIPTION}"
    MACOSX_BUNDLE_BUNDLE_VERSION "${APPLICATION_VERSION}"
    MACOSX_BUNDLE_COPYRIGHT "${COPYRIGHT}"
    MACOSX_BUNDLE_GUI_IDENTIFIER "${CODE_IDENTIFIER}"
    MACOSX_BUNDLE_ICON_FILE "AppIcon.icns"
    MACOSX_BUNDLE_INFO_STRING "${DESCRIPTION}"
    MACOSX_BUNDLE_LONG_VERSION_STRING "${VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${VERSION}")

find_library(AUDIOUNIT_LIBRARY AudioUnit)
find_library(COCOA_LIBRARY Cocoa)
find_library(COREIMAGE_LIBRARY CoreImage)
find_library(OPENGL_LIBRARY OpenGL)

target_link_libraries(TheGreatEscapeApp
    ZXSpectrum
    TheGreatEscape
    ${AUDIOUNIT_LIBRARY}
    ${COCOA_LIBRARY}
    ${COREIMAGE_LIBRARY}
    ${OPENGL_LIBRARY})

set(SRCICONSET
    ${RESOURCES_SOURCE_DIR}/TheGreatEscape/Images.xcassets/AppIcon.appiconset)

set(TMPICONSET
    ${PROJECT_BINARY_DIR}/platform/osx/AppIcon.iconset)

find_program(ICONUTIL_EXE NAMES iconutil)
add_custom_command(
    DEPENDS ${SRCICONSET}
    OUTPUT ${GENERATED_ICNS}
    # copy Xcode project's iconset to the binary dir as 'AppIcon.iconset' then process it otherwise iconutil barfs
    COMMAND cp -R ${SRCICONSET} ${TMPICONSET}
    COMMAND ${ICONUTIL_EXE} --convert icns --output ${GENERATED_ICNS} ${TMPICONSET}
    COMMENT "Building app icon...")

# Make sure the 'Resources' directory is correctly created before we build
add_custom_command(TARGET TheGreatEscapeApp
    PRE_BUILD
    COMMAND mkdir -p ./\$<CONFIG>/TheGreatEscape.app/Contents/Resources)

# Place resource files in Resources/*
set_source_files_properties(${RESOURCES}
    PROPERTIES
    MACOSX_PACKAGE_LOCATION Resources)

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
    add_custom_command(TARGET TheGreatEscapeApp
        POST_BUILD
        COMMAND ${IBTOOL}
        --errors --warnings --notices
        --output-format human-readable-text
        --compile ./\$<CONFIG>/TheGreatEscape.app/Contents/Resources/${xib}.nib ${RESOURCES_SOURCE_DIR}/${xib}.xib
        COMMENT "Compiling ${xib}.xib")
endforeach()
