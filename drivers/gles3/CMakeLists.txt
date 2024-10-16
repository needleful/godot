project(gles3)
add_library(${PROJECT_NAME}
	rasterizer_canvas_gles3.cpp
	rasterizer_scene_gles3.cpp
	rasterizer_storage_gles3.cpp
	rasterizer_gles3.cpp
	shader_cache_gles3.cpp
	shader_compiler_gles3.cpp
	shader_gles3.cpp
)

file(GLOB shader_sources "shaders/*.glsl")
set(shader_headers ${shader_sources})
list(TRANSFORM shader_headers APPEND ".gen.h")

add_custom_command(
	OUTPUT ${shader_headers}
	COMMAND python generate_shaders.py ${shader_sources}
	DEPENDS ${shader_sources} generate_shaders.py
	VERBATIM
	COMMENT "Building shaders"
)

add_custom_target(shadergen DEPENDS ${shader_headers})
add_dependencies(${PROJECT_NAME} shadergen)