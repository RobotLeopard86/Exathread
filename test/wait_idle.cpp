//Test 11: Pool::waitIdle() blocks until the task queue is empty.

#include "exathread.hpp"

#include <atomic>
#include <cstdlib>
#include <iostream>

static std::atomic<int> counter {0};

static void slowIncrement() {
	std::this_thread::sleep_for(std::chrono::milliseconds(10));
	++counter;
}

int main() {
	auto pool = exathread::Pool::Create(2);

	constexpr int N = 8;
	counter = 0;
	for(int i = 0; i < N; ++i)
		pool->submitDetached(slowIncrement);

	//waitIdle waits until the queue is empty; after that all submitted tasks
	//should have at minimum been dequeued (and most finished).
	pool->waitIdle();

	//Spin a little to let the last dequeued-but-not-yet-finished task complete.
	auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
	while(counter.load() < N && std::chrono::steady_clock::now() < deadline)
		std::this_thread::yield();

	if(counter.load() != N) {
		std::cerr << "FAIL: after waitIdle, counter=" << counter.load()
				  << ", expected " << N << "\n";
		return -1;
	}

	std::cout << "PASS\n";
	return 0;
}
