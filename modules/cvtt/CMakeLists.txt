project(cvtt)

file(GLOB fp_sources *.cpp)
list(APPEND tp_sources
	"${thirdparty_dir}/cvtt/ConvectionKernels.cpp"
)
add_library(
	${PROJECT_NAME} 
	${fp_sources}
	${tp_sources}
)
