project(platform_windows)

add_executable(godot
	context_gl_windows.cpp
	crash_handler_windows.cpp
	godot_windows.cpp
	joypad_windows.cpp
	key_mapping_windows.cpp
	os_windows.cpp
	power_windows.cpp
	tts_windows.cpp
	windows_terminal_logger.cpp
	godot_res.rc
)