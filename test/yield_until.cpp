//Test 09: yieldUntilTrue() and yieldUntilComplete() resume correctly.

#include "exathread.hpp"

#include <atomic>
#include <cstdlib>
#include <iostream>

static std::atomic<bool> flag {false};

static exathread::VoidTask waitForFlag(std::atomic<bool>* f) {
	//Predicate is copied by value into YieldOp, so we pass a pointer rather
	//than capturing by reference.
	co_await exathread::yieldUntilTrue([f]() -> bool { return f->load(); });
}

static int slowAdd(int a, int b) {
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	return a + b;
}

static exathread::ValueTask<int> waitThenDouble(exathread::Future<int> dep) {
	co_await exathread::yieldUntilComplete(dep);
	co_return *dep * 2;
}

int main() {
	auto pool = exathread::Pool::Create(2);

	//---- yieldUntilTrue ----
	{
		flag = false;
		auto fut = pool->submit(waitForFlag, &flag);

		//Give the coroutine a chance to start and yield
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
		if(fut.checkStatus() == exathread::Status::Complete) {
			std::cerr << "FAIL: task should not be complete before flag is set\n";
			return -1;
		}

		flag = true;
		fut.await();

		if(fut.checkStatus() != exathread::Status::Complete) {
			std::cerr << "FAIL: task should be Complete after flag set\n";
			return -1;
		}
	}

	//---- yieldUntilComplete ----
	{
		auto depFut = pool->submit(slowAdd, 3, 7);
		auto fut = pool->submit(waitThenDouble, depFut);
		int result = *fut;
		if(result != 20) {
			std::cerr << "FAIL: yieldUntilComplete chain should give 20, got " << result << "\n";
			return -1;
		}
	}

	std::cout << "PASS\n";
	return 0;
}
