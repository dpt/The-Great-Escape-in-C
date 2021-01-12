set(TARGET LibTheGreatEscape)

add_library(${TARGET}
  Data/ExteriorTiles.c
  Data/Font.c
  Data/InteriorObjectDefs.c
  Data/InteriorTiles.c
  Data/ItemBitmaps.c
  Data/Map.c
  Data/Masks.c
  Data/Music.c
  Data/RoomDefs.c
  Data/SpriteBitmaps.c
  Data/Sprites.c
  Data/StaticGraphics.c
  Data/StaticTiles.c
  Data/SuperTiles.c
  Engine/Create.c
  Engine/Debug.c
  Engine/Events.c
  Engine/Input.c
  Engine/Main.c
  Engine/Menu.c
  Engine/Messages.c
  Engine/Screen.c
  Engine/Text.c
  Engine/Utils.c
  Engine/Zoombox.c
  include/TheGreatEscape/Asserts.h
  include/TheGreatEscape/Debug.h
  include/TheGreatEscape/Doors.h
  include/TheGreatEscape/Events.h
  include/TheGreatEscape/ExteriorTiles.h
  include/TheGreatEscape/Font.h
  include/TheGreatEscape/Input.h
  include/TheGreatEscape/InteriorObjectDefs.h
  include/TheGreatEscape/InteriorObjects.h
  include/TheGreatEscape/InteriorTiles.h
  include/TheGreatEscape/ItemBitmaps.h
  include/TheGreatEscape/Items.h
  include/TheGreatEscape/Main.h
  include/TheGreatEscape/Map.h
  include/TheGreatEscape/Masks.h
  include/TheGreatEscape/Menu.h
  include/TheGreatEscape/Messages.h
  include/TheGreatEscape/Music.h
  include/TheGreatEscape/Pixels.h
  include/TheGreatEscape/RoomDefs.h
  include/TheGreatEscape/Rooms.h
  include/TheGreatEscape/Routes.h
  include/TheGreatEscape/Screen.h
  include/TheGreatEscape/SpriteBitmaps.h
  include/TheGreatEscape/Sprites.h
  include/TheGreatEscape/State.h
  include/TheGreatEscape/StaticGraphics.h
  include/TheGreatEscape/StaticTiles.h
  include/TheGreatEscape/SuperTiles.h
  include/TheGreatEscape/TGEObject.h
  include/TheGreatEscape/Text.h
  include/TheGreatEscape/Tiles.h
  include/TheGreatEscape/Types.h
  include/TheGreatEscape/Utils.h
  include/TheGreatEscape/Zoombox.h
  ../../include/TheGreatEscape/TheGreatEscape.h
)

target_include_directories(${TARGET}
  PUBLIC
  ../../include/
  PRIVATE
  include/
)

if(TGE_SAVES)
  target_sources(${TARGET} PRIVATE Extend/Save.c)
  target_compile_definitions(${TARGET} PRIVATE TGE_SAVES)

  # zerotape
  add_dependencies(${TARGET} zerotape)
  target_include_directories(${TARGET} PRIVATE ${ZEROTAPE_INCLUDE})
  target_link_libraries(${TARGET} ${ZEROTAPE_LIB})
endif()
