#include "profiler.h"

#ifdef NP_PROFILER
#include "core/io/logger.h"
#include "core/os/os.h"
#endif // NP_PROFILER

#ifdef NP_PROFILER
static int64_t current_time() {
	return OS::get_singleton()->get_ticks_usec();
}
#endif

ProfileMarker::ProfileMarker(const char *p_func) {
#ifdef NP_PROFILER
	func_name = p_func;
	start_time = current_time();
#endif
}

ProfileMarker::~ProfileMarker() {
#ifdef NP_PROFILER
	ProfileToken token;
	token.func_name = func_name;
	token.start_time = start_time;
	token.end_time = current_time();
	token.special_value = -1;

	ProfilerManager::singleton->append(token);
#endif
}

ProfileTimer::ProfileTimer(const char *p_func) {
#ifdef NP_PROFILER
	func_name = p_func;
	running_timer = 0;
	start_time = current_time();
#endif
}

void ProfileTimer::start() {
#ifdef NP_PROFILER
	timer_last_start = current_time();
#endif
}

void ProfileTimer::stop() {
#ifdef NP_PROFILER
	running_timer += current_time() - timer_last_start;
#endif
}

ProfileTimer::~ProfileTimer() {
#ifdef NP_PROFILER
	ProfileToken token;
	token.func_name = func_name;
	token.start_time = start_time;
	token.special_value = running_timer;

	token.end_time = current_time();

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
		p_logger->logf("%lu\n", frame_time);
		for (int i = 0; i < size; i++) {
			int64_t start = buffer[i].start_time - time_offset;
			int64_t end = buffer[i].end_time - time_offset;
			int64_t sv = buffer[i].special_value;

			p_logger->logf("%s [%ld] : %ld, %ld\n",
					buffer[i].func_name,
					sv > 0 ? sv : end - start,
					start, end);
		}
		p_logger->logf("------\n");
	}
	size = 0;
	time_offset = current_time();
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
