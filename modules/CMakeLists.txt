add_library(modules register_module_types.gen.cpp)

foreach(mod IN LISTS enabled_modules)
	add_subdirectory(${mod})
endforeach()