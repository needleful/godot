project(jpg)
set(base_dir "${thirdparty_dir}/jpeg-compressor/")

file(GLOB fp_sources *.cpp)
list(APPEND tp_sources
	jpgd.cpp
)

list(TRANSFORM tp_sources PREPEND ${base_dir})

add_library(
	${PROJECT_NAME} 
	${fp_sources}
	${tp_sources}
)

target_include_directories(${PROJECT_NAME} PRIVATE ${base_dir})