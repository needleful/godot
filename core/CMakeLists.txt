project(core)

# cpp sources
file(GLOB core_sources *.cpp)

# Generated sources
add_custom_command(
	OUTPUT script_encryption_key.gen.cpp io/certs_compressed.gen.h
	COMMAND python ${CMAKE_SOURCE_DIR}/generate.py certs
	DEPENDS ${CMAKE_SOURCE_DIR}/generate.py
		${CMAKE_SOURCE_DIR}/thirdparty/certs/ca-certificates.crt
	VERBATIM
	COMMENT "Generating certificates")
add_custom_command(
	OUTPUT authors.gen.h donors.gen.h license.gen.h
	COMMAND python ${CMAKE_SOURCE_DIR}/generate.py legal
	DEPENDS ${CMAKE_SOURCE_DIR}/generate.py 
		${CMAKE_SOURCE_DIR}/AUTHORS.md 
		${CMAKE_SOURCE_DIR}/DONORS.md 
		${CMAKE_SOURCE_DIR}/LICENSE.txt
	VERBATIM
	COMMENT "Generating legal info")
add_custom_command(
	OUTPUT version_hash.gen.cpp version_generated.gen.h
	COMMAND python ${CMAKE_SOURCE_DIR}/generate.py version
	DEPENDS 
		${CMAKE_SOURCE_DIR}/generate.py
		${CMAKE_SOURCE_DIR}/version.py
	VERBATIM
	COMMENT "Generating version info")
add_custom_command(
	OUTPUT method_bind.gen.inc
		method_bind_ext.gen.inc
		method_bind_free_func.gen.inc
	COMMAND python ${CMAKE_SOURCE_DIR}/generate.py bindings
	DEPENDS ${CMAKE_SOURCE_DIR}/generate.py
	VERBATIM
	COMMENT "Generating bindings")

add_custom_target(coregen DEPENDS
	method_bind.gen.inc
	method_bind_ext.gen.inc
	method_bind_free_func.gen.inc
	cript_encryption_key.gen.cpp
	io/certs_compressed.gen.h
	version_hash.gen.cpp
	authors.gen.h
	donors.gen.h
	version_generated.gen.h)

add_library(${PROJECT_NAME} ${core_sources})
add_dependencies(${PROJECT_NAME} coregen)

add_subdirectory(os)
add_subdirectory(math)
add_subdirectory(crypto)
add_subdirectory(io)
add_subdirectory(bind)
