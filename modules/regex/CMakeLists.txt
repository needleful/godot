project(regex)

set(base_dir "${thirdparty_dir}/pcre2/src/")

set(fp_sources
	regex.cpp
	register_types.cpp
)
list(APPEND tp_sources
	pcre2_auto_possess.c
	pcre2_chartables.c
	pcre2_compile.c
	pcre2_config.c
	pcre2_context.c
	pcre2_convert.c
	pcre2_dfa_match.c
	pcre2_error.c
	pcre2_extuni.c
	pcre2_find_bracket.c
	pcre2_jit_compile.c
	pcre2_maketables.c
	pcre2_match.c
	pcre2_match_data.c
	pcre2_newline.c
	pcre2_ord2utf.c
	pcre2_pattern_info.c
	pcre2_script_run.c
	pcre2_serialize.c
	pcre2_string_utils.c
	pcre2_study.c
	pcre2_substitute.c
	pcre2_substring.c
	pcre2_tables.c
	pcre2_ucd.c
	pcre2_valid_utf.c
	pcre2_xclass.c
)

list(TRANSFORM tp_sources PREPEND ${base_dir})

add_library(
	${PROJECT_NAME} 
	${fp_sources}
	${tp_sources}
)

target_include_directories(${PROJECT_NAME} PRIVATE ${base_dir})

target_compile_definitions( ${PROJECT_NAME} PRIVATE
	SUPPORT_JIT
	PCRE2_CODE_UNIT_WIDTH=8
)