if (NOT WIN32)
	target_sources(drivers PRIVATE pulse-so_wrap.c)
endif()
target_sources(drivers PRIVATE audio_driver_pulseaudio.cpp)