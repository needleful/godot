set(base_dir "${thirdparty_dir}/nanosvg/")
target_sources(modules PRIVATE
	"${base_dir}/nanosvg.cc"
	image_loader_svg.cpp
	register_types.cpp
)
target_include_directories(modules PRIVATE
	${base_dir}
)