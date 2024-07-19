project(pvr)

set(base_dir "${thirdparty_dir}/pvrtccompressor/")

set(fp_sources
	image_compress_pvrtc.cpp
	register_types.cpp
	texture_loader_pvr.cpp
)

list(APPEND tp_sources
	BitScale.cpp
	MortonTable.cpp
	PvrTcDecoder.cpp
	PvrTcEncoder.cpp
	PvrTcPacket.cpp
)

list(TRANSFORM tp_sources PREPEND ${base_dir})

add_library(
	${PROJECT_NAME} 
	${fp_sources}
	${tp_sources}
)

target_include_directories(${PROJECT_NAME} PRIVATE ${base_dir})