project(drivers)

add_library(drivers register_driver_types.cpp)

if(WIN32)
	add_subdirectory(windows)
    add_subdirectory(wasapi)
else()
	add_subdirectory(unix)
	add_subdirectory(xaudio2)
endif(WIN32)

add_subdirectory(alsa)
add_subdirectory(coreaudio)
add_subdirectory(pulseaudio)

add_subdirectory(gles3)
add_subdirectory(gl_context)
add_subdirectory(png)

