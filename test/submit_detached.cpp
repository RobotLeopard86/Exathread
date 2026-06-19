//Test 04: submitDetached() fires the task and does not return a future.
//We verify the work actually ran by waiting for the pool to go idle.

#include "exathread.hpp"

#include <atomic>
#include <cstdlib>
#include <iostream>

static std::atomic<int> counter {0};

static void increment() {
	++counter;
}

int main() {
	auto pool = exathread::Pool::Create(2);

	counter = 0;
	constexpr int N = 20;
	for(int i = 0; i < N; ++i)
		pool->submitDetached(increment);

	pool->waitIdle();

	//Give threads a moment to complete their current task (waitIdle only checks queue depth)
	//Spin-wait just in case the last task was dequeued but not yet incremented
	auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
	while(counter.load() != N && std::chrono::steady_clock::now() < deadline)
		std::this_thread::yield();

	if(counter.load() != N) {
		std::cerr << "FAIL: expected counter=" << N << " after submitDetached x" << N
				  << ", got " << counter.load() << "\n";
		return -1;
	}

	std::cout << "PASS\n";
	return 0;
}
