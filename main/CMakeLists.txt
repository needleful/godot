project(main)

set(controler_db
	gamecontrollerdb.txt
	godotcontrollerdb.tx
)

add_custom_command(
	OUTPUT default_controller_mappings.gen.h
	COMMAND python generate_main.py mappings ${controller.db}
	DEPENDS splash.png ${controller_db}
	VERBATIM
	COMMENT "Generating controller bindings"
)

add_custom_command(
	OUTPUT splash.gen.h
	COMMAND python generate_main.py splash
	DEPENDS splash.png splash.png
	VERBATIM
	COMMENT "Generating splash screen"
)

add_library(main
	main.cpp
	performance.cpp
	main_timer_sync.cpp
)

add_subdirectory(tests)