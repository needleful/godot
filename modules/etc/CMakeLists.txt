project(etc)
set(base_dir "${thirdparty_dir}/etc2comp/")

file(GLOB fp_sources *.cpp)
list(APPEND tp_sources
	EtcBlock4x4.cpp
	EtcBlock4x4Encoding.cpp
	EtcBlock4x4Encoding_ETC1.cpp
	EtcBlock4x4Encoding_R11.cpp
	EtcBlock4x4Encoding_RG11.cpp
	EtcBlock4x4Encoding_RGB8A1.cpp
	EtcBlock4x4Encoding_RGB8.cpp
	EtcBlock4x4Encoding_RGBA8.cpp
	Etc.cpp
	EtcDifferentialTrys.cpp
	EtcFilter.cpp
	EtcImage.cpp
	EtcIndividualTrys.cpp
	EtcMath.cpp
	EtcSortedBlockList.cpp
)

list(TRANSFORM tp_sources PREPEND ${base_dir})

add_library(
	${PROJECT_NAME} 
	${fp_sources}
	${tp_sources}
)
add_dependencies(${PROJECT_NAME} modgen)

target_include_directories(${PROJECT_NAME} PRIVATE ${base_dir})