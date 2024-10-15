project(ogg)
set(base_dir "${thirdparty_dir}/libogg/")

list(TRANSFORM tp_sources PREPEND ${base_dir})

add_library(
	${PROJECT_NAME} 
	"${base_dir}/bitwise.c"
	"${base_dir}/framing.c"
	register_types.cpp
)
add_dependencies(${PROJECT_NAME} modgen)

target_include_directories(${PROJECT_NAME} PRIVATE 
	${base_dir}
)