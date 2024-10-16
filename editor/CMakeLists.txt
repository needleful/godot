project(editor)

file(GLOB doc_files "${CMAKE_SOURCE_DIR}/doc/classes/*.xml")

add_custom_target(
	documentation
	# I wanted to just not run the command when we're not building the editor,
	# but boy oh boy CMAKE's generator expressions really are something.
	COMMAND python generate_editor.py $<$<CONFIG:Editor>:docs>
	COMMAND_EXPAND_LISTS
	BYPRODUCTS $<$<CONFIG:Editor>:doc_data_compressed.gen.h>
	DEPENDS generate_editor.py ${doc_files}
	VERBATIM
	COMMENT "Generating documentation"
)

set(language_files
	bg ca cs de el eo es_AR es fi fr gl hu id it ja ko lv ms nb nl pl 
	pt_BR pt ro ru sk sv th tr uk vi zh_CN zh_TW
)
list(TRANSFORM language_files PREPEND "${PROJECT_SOURCE_DIR}/translations")
list(TRANSFORM language_files APPEND ".po")

add_custom_target(
	translations
	COMMAND python generate_editor.py $<$<CONFIG:Editor>:translation>
	COMMAND_EXPAND_LISTS
	BYPRODUCTS $<$<CONFIG:Editor>:editor_translations.gen.h>
	DEPENDS generate_editor.py ${language_files}
	VERBATIM
	COMMENT "Generating translations"
)

set(font_dir "${thirdparty_dir}/fonts")
file(GLOB ttf_fonts "${font_dir}/*.ttf")
file(GLOB otf_fonts "${font_dir}/*.otf")
file(GLOB woff_fonts "${font_dir}/*.woff")
file(GLOB woff2_fonts "${font_dir}/*.woff2")

add_custom_target(
	fonts
	COMMAND python generate_editor.py $<$<CONFIG:Editor>:fonts>
	COMMAND_EXPAND_LISTS
	BYPRODUCTS $<$<CONFIG:Editor>:builtin_fonts.gen.h>
	DEPENDS ttf_fonts otf_fonts woff_fonts woff2_fonts
	VERBATIM
	COMMENT "Generating fonts"
)

set(platforms windows x11)
add_custom_target(
	exporters
	COMMAND python generate_editor.py $<$<CONFIG:Editor>:exporters> ${platforms}
	COMMAND_EXPAND_LISTS
	BYPRODUCTS $<$<CONFIG:Editor>:register_exporters.gen.cpp>
	VERBATIM
	COMMENT "Generating Exporters"
)

set(exporter_files ${platforms})
list(TRANSFORM exporter_files PREPEND "${CMAKE_SOURCE_DIR}/platform/")
list(TRANSFORM exporter_files APPEND "/export/export.cpp")

file(GLOB editor_files *.cpp)
list(APPEND editor_files register_exporters.gen.cpp)
add_library(editor
	$<$<CONFIG:Editor>:${editor_files}>
	$<$<CONFIG:Editor>:${exporter_files}>)

add_dependencies(editor fonts translations documentation exporters)

add_subdirectory(collada)
add_subdirectory(doc)
add_subdirectory(fileserver)
add_subdirectory(icons)
add_subdirectory(import)
add_subdirectory(plugins)