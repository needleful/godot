project(core)

file(GLOB SRC_FILES ${PROJECT_SOURCE_DIR}/*.cpp)
add_library(${PROJECT_NAME} ${SRC_FILES})

add_subdirectory(os)
add_subdirectory(math)
add_subdirectory(crypto)
add_subdirectory(io)
add_subdirectory(bind)
