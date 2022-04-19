# CMakeLists.txt
#
# The Great Escape in C
#
# Copyright (c) David Thomas, 2022
#
# vim: sw=4 ts=8 et

add_executable(TheGreatEscapeApp WIN32 TheGreatEscapeApp/main.cpp)

if(TGE_SAVES)
    target_compile_definitions(TheGreatEscapeApp PRIVATE TGE_SAVES)
endif()
	
set_target_properties(TheGreatEscapeApp
	PROPERTIES
	LINKER_LANGUAGE CXX
#    OUTPUT_NAME "TheGreatEscape"
)
	
target_link_libraries(TheGreatEscapeApp ZXSpectrum TheGreatEscape)
