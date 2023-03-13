
#ifndef PROFILER_H
#define PROFILER_H

#include <core/int_types.h>

struct ProfileMarker {
	const char *func_name;
	int64_t start_time;

	ProfileMarker(const char *p_func);
	~ProfileMarker();
};

struct ProfileToken {
	const char *func_name;
	int64_t start_time;
	int64_t end_time;
};

class Logger;

struct ProfilerManager {
	static ProfilerManager *singleton;
	ProfileToken *buffer;
	uint64_t time_offset;
	uint32_t capacity;
	uint32_t size;

	void append(ProfileToken p_marker);
	void log_and_wipe(uint64_t frame_time, Logger *logger);
	ProfilerManager();
	~ProfilerManager();

private:
	void _realloc();
};

#ifdef DEBUG_ENABLED

#ifdef __FUNCDNAME__

#define PROFILE \
	ProfileMarker __mk__{ __FUNCDNAME__ };
#else

#define PROFILE \
	ProfileMarker __mk__{ __func__ };

#endif // __FUNCDNAME__

#else

#define PROFILE

#endif // DEBUG_ENABLED

#endif // PROFILER_H