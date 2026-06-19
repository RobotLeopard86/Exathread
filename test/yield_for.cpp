//Test 08: yieldFor() suspends a coroutine task for at least the specified duration.

#include "exathread.hpp"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>

static std::atomic<bool> reached {false};

static exathread::VoidTask delayedSet() {
	co_await exathread::yieldFor(std::chrono::milliseconds(100));
	reached = true;
}

int main() {
	auto pool = exathread::Pool::Create(2);

	reached = false;
	auto start = std::chrono::steady_clock::now();
	auto fut = pool->submit(delayedSet);
	fut.await();
	auto elapsed = std::chrono::steady_clock::now() - start;

	if(!reached.load()) {
		std::cerr << "FAIL: reached flag never set\n";
		return -1;
	}

	//Must have taken at least ~100ms
	if(elapsed < std::chrono::milliseconds(95)) {
		std::cerr << "FAIL: elapsed time too short: "
				  << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()
				  << "ms\n";
		return -1;
	}

	std::cout << "PASS\n";
	return 0;
}
