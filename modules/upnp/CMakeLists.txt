project(upnp)
set(base_dir "${thirdparty_dir}/miniupnpc/")

set(fp_sources
	upnp.cpp
	upnp_device.cpp
	register_types.cpp
)
list(APPEND tp_sources
	igd_desc_parse.c
	miniupnpc.c
	minixml.c
	minisoap.c
	minissdpc.c
	miniwget.c
	upnpcommands.c
	upnpdev.c
	upnpreplyparse.c
	connecthostport.c
	portlistingparse.c
	receivedata.c
	addr_is_reserved.c
)

list(TRANSFORM tp_sources PREPEND "${base_dir}/miniupnpc/")

add_library(
	${PROJECT_NAME} 
	${fp_sources}
	${tp_sources}
)
add_dependencies(${PROJECT_NAME} modgen)

target_include_directories(${PROJECT_NAME} PRIVATE
	${base_dir}
)
target_compile_definitions(${PROJECT_NAME} PRIVATE
	MINIUPNP_STATICLIB
	MINIUPNPC_SET_SOCKET_TIMEOUT
)