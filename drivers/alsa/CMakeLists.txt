if (NOT WIN32)
	target_sources(drivers PRIVATE asound-so_wrap.c)
endif()
target_sources(drivers PRIVATE audio_driver_alsa.cpp)