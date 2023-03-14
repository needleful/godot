

#include "profiler.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#include <profileapi.h>

#include "core/io/logger.h"
#include "core/os/memory.h"

ProfileMarker::ProfileMarker(const char *p_func) {
	func_name = p_func;
	LARGE_INTEGER ticks;
	if (QueryPerformanceCounter(&ticks)) {
		start_time = ticks.QuadPart;
	} else {
		start_time = -1;
	}
}

ProfileMarker::~ProfileMarker() {
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
}

ProfileTimer::ProfileTimer(const char *p_func) {
	func_name = p_func;
	running_timer = 0;
	LARGE_INTEGER ticks;
	if (QueryPerformanceCounter(&ticks)) {
		start_time = ticks.QuadPart;
	} else {
		start_time = -1;
	}
}

void ProfileTimer::start() {
	LARGE_INTEGER ticks;
	if (QueryPerformanceCounter(&ticks)) {
		timer_last_start = ticks.QuadPart;
	} else {
		timer_last_start = -1;
	}
}

void ProfileTimer::stop() {
	LARGE_INTEGER ticks;
	if (QueryPerformanceCounter(&ticks)) {
		running_timer += ticks.QuadPart - timer_last_start;
	}
}

ProfileTimer::~ProfileTimer() {
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
}

ProfilerManager *ProfilerManager::singleton = nullptr;

ProfilerManager::ProfilerManager() {
	ProfilerManager::singleton = this;
	capacity = 1024;
	size = 0;
	buffer = (ProfileToken *)memalloc(sizeof(ProfileToken) * capacity);
	time_offset = 0;
}

void ProfilerManager::_realloc() {
	buffer = (ProfileToken *)memrealloc(buffer, sizeof(ProfileToken) * capacity);
}

void ProfilerManager::append(ProfileToken p_marker) {
	size += 1;
	if (size >= capacity) {
		capacity *= 2;
		_realloc();
	}
	buffer[size - 1] = p_marker;
}

void ProfilerManager::log_and_wipe(uint64_t frame_time, Logger *p_logger) {
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
}

ProfilerManager::~ProfilerManager() {
	memfree(buffer);
	buffer = nullptr;
	size = 0;
	capacity = 0;
}