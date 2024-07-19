target_sources(modules PRIVATE
	"${thirdparty_dir}/misc/open-simplex-noise.c"
)

target_include_directories(modules PRIVATE 
	"${thirdparty_dir}/misc/"
)