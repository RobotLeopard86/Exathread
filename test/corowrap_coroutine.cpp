//Test 20: corowrap handles functions that are already coroutines.
//When the submitted function returns VoidTask or ValueTask<T>, corowrap
//wraps it in a trampoline that awaits the inner coroutine and propagates
//both the return value and any exception. This exercises the
//`if constexpr(std::is_base_of_v<Task, R>)` branch in corowrap.

#include "exathread.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>

//A coroutine that returns void
static exathread::VoidTask voidCoro(int x) {
	co_await exathread::yieldFor(std::chrono::milliseconds(10));
	(void)x;
	co_return;
}

//A coroutine that returns a value
static exathread::ValueTask<int> valueCoro(int x) {
	co_await exathread::yieldFor(std::chrono::milliseconds(10));
	co_return x * 3;
}

//A coroutine that throws
static exathread::ValueTask<int> failingCoro() {
	co_await exathread::yieldFor(std::chrono::milliseconds(10));
	throw std::runtime_error("coro failed");
	co_return 0;
}

//A coroutine that throws void
static exathread::VoidTask failingVoidCoro() {
	co_await exathread::yieldFor(std::chrono::milliseconds(10));
	throw std::runtime_error("void coro failed");
	co_return;
}

int main() {
	auto pool = exathread::Pool::Create(2);

	//---- submit a VoidTask-returning coroutine ----
	{
		auto fut = pool->submit(voidCoro, 42);
		fut.await();
		if(fut.checkStatus() != exathread::Status::Complete) {
			std::cerr << "FAIL: VoidTask coroutine should complete successfully\n";
			return -1;
		}
	}

	//---- submit a ValueTask-returning coroutine ----
	{
		auto fut = pool->submit(valueCoro, 7);
		int result = *fut;
		if(result != 21) {
			std::cerr << "FAIL: ValueTask coroutine should return 21, got " << result << "\n";
			return -1;
		}
	}

	//---- exception propagated out of a ValueTask coroutine ----
	{
		auto fut = pool->submit(failingCoro);
		fut.await();
		if(fut.checkStatus() != exathread::Status::Failed) {
			std::cerr << "FAIL: failing ValueTask coroutine should have Failed status\n";
			return -1;
		}
		bool caught = false;
		try {
			(void)*fut;
		} catch(const std::runtime_error& e) {
			caught = true;
			if(std::string(e.what()) != "coro failed") {
				std::cerr << "FAIL: wrong exception message: " << e.what() << "\n";
				return -1;
			}
		}
		if(!caught) {
			std::cerr << "FAIL: should have rethrown from failing ValueTask coroutine\n";
			return -1;
		}
	}

	//---- exception propagated out of a VoidTask coroutine ----
	{
		auto fut = pool->submit(failingVoidCoro);
		fut.await();
		if(fut.checkStatus() != exathread::Status::Failed) {
			std::cerr << "FAIL: failing VoidTask coroutine should have Failed status\n";
			return -1;
		}
	}

	//---- chaining: coroutine -> then -> coroutine ----
	{
		auto fut = pool->submit(valueCoro, 3);//returns 9
		auto fut2 = fut.then(valueCoro);	  //9 * 3 = 27
		if(*fut2 != 27) {
			std::cerr << "FAIL: chained coroutines should give 27, got " << *fut2 << "\n";
			return -1;
		}
	}

	std::cout << "PASS\n";
	return 0;
}
