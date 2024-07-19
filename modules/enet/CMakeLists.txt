project(enet)
set(base_dir "${thirdparty_dir}/${PROJECT_NAME}/")

set(fp_sources
	networked_multiplayer_enet.cpp
	register_types.cpp
)
list(APPEND tp_sources
	godot.cpp
	callbacks.c
	compress.c
	host.c
	list.c
	packet.c
	peer.c
	protocol.c
)

list(TRANSFORM tp_sources PREPEND ${base_dir})

add_library(
	${PROJECT_NAME} 
	${fp_sources}
	${tp_sources}
)

target_include_directories(${PROJECT_NAME} PRIVATE ${base_dir})