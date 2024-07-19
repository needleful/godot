file(GLOB sources *.cpp)
add_library(tests
	$<$<CONFIG:Editor>:${sources}>
)