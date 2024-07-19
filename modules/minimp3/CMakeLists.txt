project(minimp3)
set(base_dir "${thirdparty_dir}/${PROJECT_NAME}/")

file(GLOB fp_sources *.cpp)
add_library(
    ${PROJECT_NAME} 
    ${fp_sources}
)

target_include_directories(${PROJECT_NAME} PRIVATE ${base_dir})