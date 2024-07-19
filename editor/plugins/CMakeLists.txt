file(GLOB sources *.cpp)
target_sources(editor PRIVATE
	$<$<CONFIG:Editor>:${sources}>
)
