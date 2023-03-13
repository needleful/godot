

#include "profiler.h"

#define WIN32_LEAN_AND_MEAN
#include <profileapi.h>
#include <windows.h>

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
			p_logger->logf("%s : %d, %d\n",
					buffer[i].func_name,
					buffer[i].start_time - time_offset,
					buffer[i].end_time - time_offset);
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