project(minimp3)
set(base_dir "${thirdparty_dir}/${PROJECT_NAME}/")

file(GLOB fp_sources *.cpp)
add_library(
    ${PROJECT_NAME} 
    ${fp_sources}
)
add_dependencies(${PROJECT_NAME} modgen)

target_include_directories(${PROJECT_NAME} PRIVATE ${base_dir})