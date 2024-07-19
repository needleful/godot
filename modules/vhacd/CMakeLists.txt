project(vhacd)
set(base_dir "${thirdparty_dir}/${PROJECT_NAME}/")

set(fp_sources
	register_types.cpp
)
list(APPEND tp_sources
	vhacdManifoldMesh.cpp
	FloatMath.cpp
	vhacdMesh.cpp
	vhacdICHull.cpp
	vhacdVolume.cpp
	VHACD-ASYNC.cpp
	btAlignedAllocator.cpp
	vhacdRaycastMesh.cpp
	VHACD.cpp
	btConvexHullComputer.cpp
)

list(TRANSFORM tp_sources PREPEND "${base_dir}/src/")

add_library(
	${PROJECT_NAME} 
	${fp_sources}
	${tp_sources}
)

target_include_directories(${PROJECT_NAME} PRIVATE
	"${base_dir}/inc/"
)