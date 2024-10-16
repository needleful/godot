project(core)

## Generated sources
add_custom_command(
	OUTPUT script_encryption_key.gen.cpp io/certs_compressed.gen.h
	COMMAND python ${CMAKE_SOURCE_DIR}/generate.py certs
	DEPENDS ${CMAKE_SOURCE_DIR}/generate.py
		${thirdparty_dir}/certs/ca-certificates.crt
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

## Third-party libraries
set(thirdparty_sources
    fastlz.c
    smaz.c
    # C++ sources
    hq2x.cpp
    pcg.cpp
    triangulator.cpp
    clipper.cpp)

list(TRANSFORM thirdparty_sources PREPEND misc/)

if(zlib IN_LIST builtin_libraries)
	list(APPEND thirdparty_sources
		zlib/adler32.c
		zlib/compress.c
		zlib/crc32.c
		zlib/deflate.c
		zlib/infback.c
		zlib/inffast.c
		zlib/inflate.c
		zlib/inftrees.c
		zlib/trees.c
		zlib/uncompr.c
		zlib/zutil.c
	)
	add_compile_definitions(
		$<$<CONFIG:Editor,ReleaseDebug>:
			ZLIB_DEBUG
		>
	)
endif()

if(zstd IN_LIST builtin_libraries)
	list(APPEND thirdparty_sources
		zstd/common/debug.c
        zstd/common/entropy_common.c
        zstd/common/error_private.c
        zstd/common/fse_decompress.c
        zstd/common/pool.c
        zstd/common/threading.c
        zstd/common/xxhash.c
        zstd/common/zstd_common.c
        zstd/compress/fse_compress.c
        zstd/compress/hist.c
        zstd/compress/huf_compress.c
        zstd/compress/zstd_compress.c
        zstd/compress/zstd_double_fast.c
        zstd/compress/zstd_fast.c
        zstd/compress/zstd_lazy.c
        zstd/compress/zstd_ldm.c
        zstd/compress/zstd_opt.c
        zstd/compress/zstdmt_compress.c
        zstd/compress/zstd_compress_literals.c
        zstd/compress/zstd_compress_sequences.c
        zstd/compress/zstd_compress_superblock.c
        zstd/decompress/huf_decompress.c
        zstd/decompress/zstd_ddict.c
        zstd/decompress/zstd_decompress_block.c
        zstd/decompress/zstd_decompress.c
    )
	if(NOT WIN32)
		# This isn't necessarily true, but we only have Windows and Linux right now
		list(APPEND thirdparty_sources decompress/huf_decompress_amd64.S)
	endif()

	add_compile_definitions(
		ZSTD_STATIC_LINKING_ONLY
	)
	set(zstd_dir ${thirdparty_dir}/zstd)
	include_directories(${zstd_dir} ${zstd_dir}/common)
endif()

if(mbedtls IN_LIST builtin_libraries)
	include_directories(${thirdparty_dir}/mbedtls/include/)
endif()

list(APPEND thirdparty_sources
	minizip/ioapi.c
	minizip/unzip.c
	minizip/zip.c
)

list(TRANSFORM thirdparty_sources PREPEND ${thirdparty_dir}/)
add_library(thirdparty ${thirdparty_sources})

## The core package
file(GLOB core_sources *.cpp)
add_library(${PROJECT_NAME} ${core_sources})
add_dependencies(${PROJECT_NAME} coregen)

## Sub-projects
add_subdirectory(os)
add_subdirectory(math)
add_subdirectory(crypto)
add_subdirectory(io)
add_subdirectory(bind)
