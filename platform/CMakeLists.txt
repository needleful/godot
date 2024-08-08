add_library(platform register_platform_apis.cpp)

if(WIN32)
	add_subdirectory(windows)
else()
	add_subdirectory(x11)
endif()