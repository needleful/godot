cmake_minimum_required(VERSION 3.28)
project(Godot)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

option(editor "Build the editor tools")

add_subdirectory(core)
add_subdirectory(servers)
add_subdirectory(scene)
add_subdirectory(platform)
add_subdirectory(modules)
add_subdirectory(main)

if(editor)
	add_subdirectory(tools)
endif()
