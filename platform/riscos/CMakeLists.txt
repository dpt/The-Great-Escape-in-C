if (NOT DEFINED ENV{GCCSDK_INSTALL_ENV})
    message(FATAL_ERROR "GCCSDK_INSTALL_ENV is not set")
endif()

if (NOT DEFINED ENV{APPENGINE_ROOT})
    message(FATAL_ERROR "APPENGINE_ROOT is not set")
endif()

set(TARGET ${PROJECT_NAME})

set(SOURCES
  ./!GtEscape/bitfifo.c
  ./!GtEscape/bitfifo.h
  ./!GtEscape/dataxfer.c
  ./!GtEscape/dataxfer.h
  ./!GtEscape/globals.c
  ./!GtEscape/globals.h
  ./!GtEscape/iconbar.c
  ./!GtEscape/iconbar.h
  ./!GtEscape/iconnames.h
  ./!GtEscape/main.c
  ./!GtEscape/menunames.h
  ./!GtEscape/poll.c
  ./!GtEscape/poll.h
  ./!GtEscape/ssbuffer.h
  ./!GtEscape/timermod.c
  ./!GtEscape/timermod.h
  ./!GtEscape/zxgame.c
  ./!GtEscape/zxgame.h
  ./!GtEscape/zxgames.c
  ./!GtEscape/zxgames.h
  ./!GtEscape/zxscale.c
  ./!GtEscape/zxscale.h
)

# Avoid a warning from CMake
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

add_executable(${TARGET}
  ${SOURCES}
)

set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME_MINSIZEREL !RunImageMn,ff8)
set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME_RELEASE !RunImage,ff8)
set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME_DEBUG !RunImageDb,ff8)

# OSLib
target_include_directories(${TARGET} PRIVATE
    $ENV{GCCSDK_INSTALL_ENV}/include)
target_link_libraries(${TARGET}
    $ENV{GCCSDK_INSTALL_ENV}/lib/libOSLib32.a)

# AppEngine, incl. Fortify
target_include_directories(${TARGET} PRIVATE
    $ENV{APPENGINE_ROOT}/libs)
target_link_libraries(${TARGET}
    $ENV{APPENGINE_ROOT}/libs/appengine/appengine.a
    $ENV{APPENGINE_ROOT}/libs/fortify/libfortify.a)

if(TGE_SAVES)
  target_compile_definitions(${TARGET} PRIVATE TGE_SAVES)
  target_sources(${TARGET} PRIVATE
      ./!GtEscape/zxsave.c
      ./!GtEscape/zxsave.h
  )
endif()

target_link_libraries(${TARGET}
  LibZXSpectrum
  LibTheGreatEscape
)

# Install into build directory
set(CMAKE_INSTALL_PREFIX . CACHE PATH "CMake install prefix" FORCE)

install(TARGETS TheGreatEscape
	CONFIGURATIONS MinSizeRel Release Debug
	RUNTIME DESTINATION !GtEscape)
install(FILES
	./!GtEscape/!Boot,feb
	./!GtEscape/!Run,feb
	./!GtEscape/!Help,feb
	./!GtEscape/!Sprites,ff9
	./!GtEscape/!Sprites22,ff9
	./!GtEscape/AppEngine,ffa
	./!GtEscape/TimerMod,ffa
	DESTINATION !GtEscape)
install(DIRECTORY
	./!GtEscape/Resources
	DESTINATION !GtEscape)
