project(theora)
set(base_dir "${thirdparty_dir}/libtheora/")

set(fp_sources
	video_stream_theora.cpp
	register_types.cpp
)
list(APPEND tp_sources
	bitpack.c
	cpu.c
	decinfo.c
	decode.c
	dequant.c
	fragment.c
	huffdec.c
	idct.c
	info.c
	internal.c
	quant.c
	state.c 
)

list(TRANSFORM tp_sources PREPEND ${base_dir})

add_library(
	${PROJECT_NAME} 
	${fp_sources}
	${tp_sources}
)
add_dependencies(${PROJECT_NAME} modgen)

target_include_directories(${PROJECT_NAME} PRIVATE
	"${base_dir}"
	"${thirdparty_dir}/libogg"
	"${thirdparty_dir}/libvorbis"
)