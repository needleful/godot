// My new thing

#ifndef CONCURRENT_QUEUE_H
#define CONCURRENT_QUEUE_H

#include <queue>

#include "core/os/mutex.h"
#include "core/os/thread.h"

template <class T>
class ConcurrentQueue {
protected:
	Mutex mtx;
	std::queue<T> queue;

public:
	void push(T elem) {
		MutexLock lock(mtx);
		queue.push(elem);
	}
	void pop(T &value) {
		MutexLock lock(mtx);
		value = queue.front();
		queue.pop();
	}

	bool empty() const {
		return queue.empty();
	}
};

#endif // CONCURRENT_QUEUE_H