//Test 02: submit() with a void function, then Future::await() and checkStatus()

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

	//---- Single void submission ----
	counter = 0;
	{
		auto fut = pool->submit(increment);
		fut.await();

		if(counter.load() != 1) {
			std::cerr << "FAIL: counter should be 1 after single submit, got " << counter.load() << "\n";
			return -1;
		}
		if(fut.checkStatus() != exathread::Status::Complete) {
			std::cerr << "FAIL: future status should be Complete\n";
			return -1;
		}
	}

	//---- Multiple void submissions ----
	counter = 0;
	{
		constexpr int N = 10;
		std::vector<exathread::Future<void>> futs;
		futs.reserve(N);
		for(int i = 0; i < N; ++i)
			futs.push_back(pool->submit(increment));
		for(auto& f : futs)
			f.await();

		if(counter.load() != N) {
			std::cerr << "FAIL: counter should be " << N << " after " << N << " submits, got " << counter.load() << "\n";
			return -1;
		}
	}

	std::cout << "PASS\n";
	return 0;
}
