file(GLOB sources *.cpp)
target_sources(scene PRIVATE
	"${thirdparty_dir}/misc/mikktspace.c"
	${sources}
)
add_dependencies(scene thirdparty)

add_subdirectory(default_theme)