file(GLOB sources LIST_DIRECTORIES false *.cpp)
target_sources(scene PRIVATE ${sources})

file(GLOB portal_sources LIST_DIRECTORIES false portals/*.cpp)
target_sources(scene PRIVATE ${portal_sources})