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

add_executable(${TARGET}
  ${SOURCES}
)

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
