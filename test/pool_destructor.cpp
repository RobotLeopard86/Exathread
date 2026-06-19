//Test 12: Destroying the pool waits for all enqueued tasks to finish.

#include "exathread.hpp"

#include <atomic>
#include <cstdlib>
#include <iostream>

static std::atomic<int> counter {0};

static void increment() {
	std::this_thread::sleep_for(std::chrono::milliseconds(5));
	++counter;
}

int main() {
	constexpr int N = 10;
	counter = 0;
	{
		auto pool = exathread::Pool::Create(2);
		for(int i = 0; i < N; ++i)
			pool->submitDetached(increment);
		//Pool goes out of scope here; destructor calls waitIdle() then joins threads.
	}

	//By here the destructor has returned, so all tasks must be done.
	//Allow a tiny grace window for the very last task that was dequeued just
	//before the stop request.
	auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
	while(counter.load() < N && std::chrono::steady_clock::now() < deadline)
		std::this_thread::yield();

	if(counter.load() != N) {
		std::cerr << "FAIL: after pool destruction, counter=" << counter.load()
				  << ", expected " << N << "\n";
		return -1;
	}

	std::cout << "PASS\n";
	return 0;
}
