cmake_minimum_required(VERSION 3.28)
set(CMAKE_CONFIGURATION_TYPES "Editor;Release;ReleaseDebug" CACHE STRING "" FORCE)
project(Godot)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++20 /Wall")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++20 -Wall -Wextra -Wpedantic")
endif(MSVC)

add_compile_definitions(
	OPENGL_ENABLED
	DISABLE_DEPRECATED
	$<$<CONFIG:Editor,ReleaseDebug>:
		CRASH_HANDLER_EXCEPTION
		DEBUG_ENABLED
		NP_PROFILER
	>
	$<$<CONFIG:Editor>:TOOLS_ENABLED>
)

add_subdirectory(editor)

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
