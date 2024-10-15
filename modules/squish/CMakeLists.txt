project(squish)
set(base_dir "${thirdparty_dir}/${PROJECT_NAME}/")

set(fp_sources
	image_compress_squish.cpp
	register_types.cpp
)
list(APPEND tp_sources
	alpha.cpp
	clusterfit.cpp
	colourblock.cpp
	colourfit.cpp
	colourset.cpp
	maths.cpp
	rangefit.cpp
	singlecolourfit.cpp
	squish.cpp
)

list(TRANSFORM tp_sources PREPEND ${base_dir})

add_library(
	${PROJECT_NAME} 
	${fp_sources}
	${tp_sources}
)
add_dependencies(${PROJECT_NAME} modgen)

target_include_directories(${PROJECT_NAME} PRIVATE ${base_dir})