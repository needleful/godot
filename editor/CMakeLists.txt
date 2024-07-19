file(GLOB editor_files *.cpp)
add_library(editor $<$<CONFIG:Editor>:${editor_files}>)

add_subdirectory(collada)
add_subdirectory(doc)
add_subdirectory(fileserver)
add_subdirectory(icons)
add_subdirectory(import)
add_subdirectory(plugins)