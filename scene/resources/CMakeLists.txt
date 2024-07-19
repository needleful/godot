file(GLOB sources *.cpp)
target_sources(scene PRIVATE
	"${thirdparty_dir}/misc/mikktspace.c"
	${sources}
)

add_subdirectory(default_theme)