target_sources(drivers PRIVATE
	"${thirdparty_dir}/glad/glad.c")

add_compile_definitions(
	OPENGL_ENABLED
	DISABLE_DEPRECATED
)