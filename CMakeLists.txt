cmake_minimum_required(VERSION 3.28)
project(Godot)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++20")
endif(MSVC)

option(editor "Build the editor tools")

add_compile_definitions(OPENGL_ENABLED)
if(editor)
	add_compile_definitions(CRASH_HANDLER_EXCEPTION)
endif()

# What are you gonna do about it, punk?
include_directories(${PROJECT_SOURCE_DIR})

add_subdirectory(core)
add_subdirectory(drivers)
add_subdirectory(servers)
add_subdirectory(scene)
add_subdirectory(platform)
add_subdirectory(modules)
add_subdirectory(main)

if(editor)
	add_subdirectory(tools)
endif()
