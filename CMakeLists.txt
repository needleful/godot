cmake_minimum_required(VERSION 3.28)
project(Godot)

set(CMAKE_CONFIGURATION_TYPES "Editor;Release;ReleaseDebug" CACHE STRING "" FORCE)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std:c++17 -Wall -Wextra -Wpedantic")
endif(MSVC)

add_compile_definitions(
	$<$<CONFIG:Editor,ReleaseDebug>:
		CRASH_HANDLER_EXCEPTION
		DEBUG_ENABLED
		NP_PROFILER
	>
	$<$<CONFIG:Editor>:TOOLS_ENABLED>
)

set(thirdparty_dir "${CMAKE_SOURCE_DIR}/thirdparty")

# What are you gonna do about it, punk?
include_directories(
	"${PROJECT_SOURCE_DIR}"
	"${thirdparty_dir}/libpng"
	"${thirdparty_dir}/glad"
	"${thirdparty_dir}/freetype/include"
)
if(WIN32)
	include_directories(platform/windows)
else()
	include_directories(platform/x11)
endif()

set(enabled_modules
	bmp
	bullet
	csg
	cvtt
	dds
	enet
	etc
	freetype
	gdscript
	gltf
	hdr
	jpg
	jsonrpc
	mbedtls
	minimp3
	navigation
	np_multi_mesh
	ogg
	opensimplex
	opus
	pvr
	regex
	squish
	stb_vorbis
	svg
	tga
	theora
	tinyexr
	upnp
	vhacd
	vorbis
)

set(builtin_libraries
	bullet
	certs
	embree
	enet
	freetype
	libogg
	libpng
	libtheora
	libvorbis
	libvpx
	libwebp
	wslay
	mbedtls
	miniupnpc
	opus
	pcre2
	pcre2_with_jit
	recast
	rvo2
	squish
	xatlas
	zlib
	zstd
)

add_subdirectory(core)
add_subdirectory(drivers)
add_subdirectory(editor)
add_subdirectory(main)
add_subdirectory(modules)
add_subdirectory(platform)
add_subdirectory(scene)
add_subdirectory(servers)
