project(freetype)
set(base_dir "${thirdparty_dir}/${PROJECT_NAME}/")

file(GLOB fp_sources *.cpp)
list(APPEND tp_sources
	src/autofit/autofit.c
	src/base/ftbase.c
	src/base/ftbbox.c
	src/base/ftbdf.c
	src/base/ftbitmap.c
	src/base/ftcid.c
	src/base/ftdebug.c
	src/base/ftfstype.c
	src/base/ftgasp.c
	src/base/ftglyph.c
	src/base/ftgxval.c
	src/base/ftinit.c
	src/base/ftmm.c
	src/base/ftotval.c
	src/base/ftpatent.c
	src/base/ftpfr.c
	src/base/ftstroke.c
	src/base/ftsynth.c
	src/base/ftsystem.c
	src/base/fttype1.c
	src/base/ftwinfnt.c
	src/bdf/bdf.c
	src/bzip2/ftbzip2.c
	src/cache/ftcache.c
	src/cff/cff.c
	src/cid/type1cid.c
	src/gxvalid/gxvalid.c
	src/gzip/ftgzip.c
	src/lzw/ftlzw.c
	src/otvalid/otvalid.c
	src/pcf/pcf.c
	src/pfr/pfr.c
	src/psaux/psaux.c
	src/pshinter/pshinter.c
	src/psnames/psnames.c
	src/raster/raster.c
	src/sdf/sdf.c
	src/svg/svg.c
	src/smooth/smooth.c
	src/truetype/truetype.c
	src/type1/type1.c
	src/type42/type42.c
	src/winfonts/winfnt.c
)

list(APPEND brotli_sources
	common/constants.c
	common/context.c
	common/dictionary.c
	common/platform.c
	common/shared_dictionary.c
	common/transform.c
	dec/bit_reader.c
	dec/decode.c
	dec/huffman.c
	dec/state.c
)

list(TRANSFORM tp_sources PREPEND ${base_dir})
list(TRANSFORM brotli_sources PREPEND "${thirdparty_dir}/brotli/")

add_library(
	${PROJECT_NAME} 
	${fp_sources}
	${tp_sources}
	${brotli_sources}
)

target_include_directories(${PROJECT_NAME} PRIVATE
	"${base_dir}/include"
	"${thirdparty_dir}/libpng"
)

target_compile_definitions(${PROJECT_NAME} PRIVATE 
	FT_CONFIG_OPTION_USE_BROTLI
	FT2_BUILD_LIBRARY
	FT_CONFIG_OPTION_USE_PNG
	FT_CONFIG_OPTION_SYSTEM_ZLIB
	$<$<CONFIG:Editor,ReleaseDebug>:ZLIB_DEBUG>
)