project(vorbis)
set(base_dir "${thirdparty_dir}/lib${PROJECT_NAME}/")

set(fp_sources
	register_types.cpp
)
list(APPEND tp_sources
	bitrate.c
	block.c
	codebook.c
	envelope.c
	floor0.c
	floor1.c
	info.c
	lookup.c
	lpc.c
	lsp.c
	mapping0.c
	mdct.c
	psy.c
	registry.c
	res0.c
	sharedbook.c
	smallft.c
	synthesis.c
	vorbisfile.c
	window.c
)

list(TRANSFORM tp_sources PREPEND "${base_dir}/")

add_library(
	${PROJECT_NAME} 
	${fp_sources}
	${tp_sources}
)

target_include_directories(${PROJECT_NAME} PRIVATE
	${base_dir}
	"${thirdparty_dir}/libogg"
)