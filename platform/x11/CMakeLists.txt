project(platform_linux)

add_executable(godot
    context_gl_x11.cpp
    crash_handler_x11.cpp
    os_x11.cpp
    key_mapping_x11.cpp
    joypad_linux.cpp
    power_x11.cpp
    detect_prime.cpp
    godot_x11.cpp
    libudev-so_wrap.c
    # Text-to-speech
    speechd-so_wrap.c
    tts_linux.cpp
)