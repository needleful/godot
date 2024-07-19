set(base_dir "${thirdparty_dir}/tinyexr/")
target_sources(modules PRIVATE
	"${base_dir}/tinyexr.cc"
	image_loader_tinyexr.cpp
	image_saver_tinyexr.cpp
	register_types.cpp
)

target_include_directories(modules PRIVATE 
	"${base_dir}"
)
target_compile_definitions(modules PRIVATE
	TINYEXR_USE_THREAD
	TINYEXR_USE_MINIZ
)