add_custom_command(
	OUTPUT modules_enabled.gen.h modules_enabled.gen.cpp register_module_types.gen.cpp
	COMMAND "echo generate_modules.py ${enabled_modules}")

add_library(modules 
	register_module_types.gen.cpp
	modules_enabled.gen.cpp)

foreach(mod IN LISTS enabled_modules)
	add_subdirectory(${mod})
endforeach()