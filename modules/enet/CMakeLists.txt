project(enet)
set(base_dir "${thirdparty_dir}/${PROJECT_NAME}/")

file(GLOB fp_sources *.cpp)
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