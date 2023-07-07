

#include "profiler.h"

#ifdef NP_PROFILER
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#include "core/io/logger.h"
#include "core/os/memory.h"
#include <profileapi.h>
#endif // NP_PROFILER

ProfileMarker::ProfileMarker(const char *p_func) {
#ifdef NP_PROFILER
	func_name = p_func;
	LARGE_INTEGER ticks;
	if (QueryPerformanceCounter(&ticks)) {
		start_time = ticks.QuadPart;
	} else {
		start_time = -1;
	}
#endif
}

ProfileMarker::~ProfileMarker() {
#ifdef NP_PROFILER
	ProfileToken token;
	token.func_name = func_name;
	token.start_time = start_time;
	token.special_value = -1;

	LARGE_INTEGER ticks;
	if (QueryPerformanceCounter(&ticks)) {
		token.end_time = ticks.QuadPart;
	}

	else {
		token.end_time = -1;
	}

	ProfilerManager::singleton->append(token);
#endif
}

ProfileTimer::ProfileTimer(const char *p_func) {
#ifdef NP_PROFILER
	func_name = p_func;
	running_timer = 0;
	LARGE_INTEGER ticks;
	if (QueryPerformanceCounter(&ticks)) {
		start_time = ticks.QuadPart;
	} else {
		start_time = -1;
	}
#endif
}

void ProfileTimer::start() {
#ifdef NP_PROFILER
	LARGE_INTEGER ticks;
	if (QueryPerformanceCounter(&ticks)) {
		timer_last_start = ticks.QuadPart;
	} else {
		timer_last_start = -1;
	}
#endif
}

void ProfileTimer::stop() {
#ifdef NP_PROFILER
	LARGE_INTEGER ticks;
	if (QueryPerformanceCounter(&ticks)) {
		running_timer += ticks.QuadPart - timer_last_start;
	}
#endif
}

ProfileTimer::~ProfileTimer() {
#ifdef NP_PROFILER
	ProfileToken token;
	token.func_name = func_name;
	token.start_time = start_time;
	token.special_value = running_timer;

	LARGE_INTEGER ticks;
	if (QueryPerformanceCounter(&ticks)) {
		token.end_time = ticks.QuadPart;
	}

	else {
		token.end_time = -1;
	}

	ProfilerManager::singleton->append(token);
#endif
}

ProfilerManager *ProfilerManager::singleton = nullptr;

ProfilerManager::ProfilerManager() {
#ifdef NP_PROFILER
	ProfilerManager::singleton = this;
	capacity = 1024;
	size = 0;
	buffer = (ProfileToken *)Memory::alloc_static(sizeof(ProfileToken) * capacity, true);
	time_offset = 0;
#endif
}

void ProfilerManager::_realloc() {
#ifdef NP_PROFILER
	buffer = (ProfileToken *)Memory::realloc_static(buffer, sizeof(ProfileToken) * capacity, true);
#endif
}

void ProfilerManager::append(ProfileToken p_marker) {
#ifdef NP_PROFILER
	size += 1;
	if (size >= capacity) {
		capacity *= 2;
		_realloc();
	}
	buffer[size - 1] = p_marker;
#endif
}

void ProfilerManager::log_and_wipe(uint64_t frame_time, Logger *p_logger) {
#ifdef NP_PROFILER
	if (p_logger) {
		p_logger->logf("%u\n", frame_time);
		for (int i = 0; i < size; i++) {
			int64_t start = buffer[i].start_time - time_offset;
			int64_t end = buffer[i].end_time - time_offset;
			int64_t sv = buffer[i].special_value;

			p_logger->logf("%s [%d] : %d, %d\n",
					buffer[i].func_name,
					sv > 0 ? sv : end - start,
					start, end);
		}
		p_logger->logf("------\n");
	}
	size = 0;
	LARGE_INTEGER ticks;
	if (QueryPerformanceCounter(&ticks)) {
		time_offset = ticks.QuadPart;
	}

	else {
		time_offset = 0;
	}
#endif
}

ProfilerManager::~ProfilerManager() {
#ifdef NP_PROFILER
	Memory::free_static(buffer, true);
	buffer = nullptr;
	size = 0;
	capacity = 0;
#endif
}