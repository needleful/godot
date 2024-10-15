project(core)

# cpp sources
file(GLOB core_sources ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
# Generated sources
add_custom_command(
	OUTPUT script_encryption_key.gen.cpp io/certs_compressed.gen.h
	COMMAND python generate.py certs
	DEPENDS generate.py
		${PROJECT_SOURCE_DIR}/thirdparty/certs/ca-certificates.crt
	VERBATIM)
add_custom_command(
	OUTPUT version_hash.gen.cpp authors.gen.h donors.gen.h license.gen.h
	COMMAND python generate.py info
	DEPENDS generate.py 
		${PROJECT_SOURCE_DIR}/AUTHORS.md 
		${PROJECT_SOURCE_DIR}/DONORS.md 
		${PROJECT_SOURCE_DIR}/LICENSE.txt
	VERBATIM)
add_custom_command(
	OUTPUT method_bind.gen.inc
		method_bind_ext.gen.inc
		method_bind_free_func.gen.inc
	COMMAND python generate.py bindings
	DEPENDS generate.py
	VERBATIM)

add_custom_target(coregen DEPENDS
	method_bind.gen.inc
	method_bind_ext.gen.inc
	method_bind_free_func.gen.inc
	cript_encryption_key.gen.cpp
	io/certs_compressed.gen.h
	version_hash.gen.cpp
	authors.gen.h
	donors.gen.h)

add_library(${PROJECT_NAME} ${core_sources})
add_dependencies(${PROJECT_NAME} coregen)

add_subdirectory(os)
add_subdirectory(math)
add_subdirectory(crypto)
add_subdirectory(io)
add_subdirectory(bind)
