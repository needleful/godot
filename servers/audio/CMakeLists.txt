file(GLOB sources LIST_DIRECTORIES false *.cpp)
target_sources(scene PRIVATE ${sources})

file(GLOB effect_sources LIST_DIRECTORIES false effects/*.cpp)
target_sources(scene PRIVATE ${effect_sources})