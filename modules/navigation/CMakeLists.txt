project(navigation)

set(recast_dir "${thirdparty_dir}/recastnavigation/Recast/")

file(GLOB fp_sources *.cpp)

list(APPEND recast_sources
	Source/Recast.cpp
	Source/RecastAlloc.cpp
	Source/RecastArea.cpp
	Source/RecastAssert.cpp
	Source/RecastContour.cpp
	Source/RecastFilter.cpp
	Source/RecastLayers.cpp
	Source/RecastMesh.cpp
	Source/RecastMeshDetail.cpp
	Source/RecastRasterization.cpp
	Source/RecastRegion.cpp
)
set(rvo_sources 
	"${thirdparty_dir}/rvo2/Agent.cpp"
	"${thirdparty_dir}/rvo2/KdTree.cpp"
)


list(TRANSFORM recast_sources PREPEND "${recast_dor}")

add_library(
	${PROJECT_NAME} 
	${fp_sources}
	${tp_sources}
)
add_dependencies(${PROJECT_NAME} modgen)

target_include_directories(${PROJECT_NAME} PRIVATE
	"${recast_dir}/Include"
	"${thirdparty_dir}/rvo2"
)

target_compile_definitions(${PROJECT_NAME} PRIVATE 
	GODOT_ENET
)