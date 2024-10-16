add_custom_command(
	OUTPUT modules_enabled.gen.h register_module_types.gen.cpp
	COMMAND python generate_modules.py ${enabled_modules}
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/generate_modules.py
	COMMENT "Generating module sources"
	VERBATIM)

add_custom_target(modgen DEPENDS modules_enabled.gen.h register_module_types.gen.cpp)

add_library(modules)
add_dependencies(modules modgen thirdparty)

foreach(mod IN LISTS enabled_modules)
	add_subdirectory(${mod})
endforeach()