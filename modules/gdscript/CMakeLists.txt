project(gdscript)

set(sources
	gdscript.cpp
	gdscript_compiler.cpp
	gdscript_editor.cpp
	gdscript_function.cpp
	gdscript_functions.cpp
	gdscript_parser.cpp
	gdscript_tokenizer.cpp
	register_types.cpp
)

set(editor_sources
	"${PROJECT_SOURCE_DIR}/editor/gdscript_highlighter.cpp"
)

add_library(gdscript
	${sources}
	$<$<CONFIG:Editor>:${editor_sources}>
)
if(jsonrpc IN_LIST enabled_modules
	AND websocket IN_LIST enabled_modules
)
	target_sources(gdscript PRIVATE
		language_server/gdscript_extend_parser.cpp
		language_server/gdscript_language_protocol.cpp
		language_server/gdscript_language_server.cpp
		language_server/gdscript_text_document.cpp
		language_server/gdscript_workspace.cpp
	)
else()
	target_compile_definitions(gdscript PRIVATE GDSCRIPT_NO_LSP)
endif()
