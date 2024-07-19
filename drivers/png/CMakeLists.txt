file(GLOB png_sources
	"${thirdparty_dir}/libpng/*.c")
target_sources(drivers PRIVATE
	${png_sources}
	image_loader_png.cpp
	png_driver_common.cpp
	resource_saver_png.cpp
)