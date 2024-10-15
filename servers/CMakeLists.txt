add_library(servers
	audio_server.cpp
	camera_server.cpp
	navigation_2d_server.cpp
	navigation_server.cpp
	physics_2d_server.cpp
	physics_server.cpp
	visual_server.cpp
	visual_server_callbacks.cpp
	register_server_types.cpp
)

add_subdirectory(physics_2d)
add_subdirectory(visual)
add_subdirectory(audio)