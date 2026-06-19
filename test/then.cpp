//Test 07: Future::then() chaining — continuation receives prior result.

#include "exathread.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

static int produceValue() {
	return 6;
}

static int doubleValue(int x) {
	return x * 2;
}

static std::string intToStr(int x) {
	return std::to_string(x);
}

static void consumeInt(int x) {
	(void)x;
}

int main() {
	auto pool = exathread::Pool::Create(2);

	//---- int -> int chain ----
	{
		auto fut = pool->submit(produceValue);
		auto fut2 = fut.then(doubleValue);
		int result = *fut2;
		if(result != 12) {
			std::cerr << "FAIL: then chain should give 12, got " << result << "\n";
			return -1;
		}
	}

	//---- int -> string chain ----
	{
		auto fut = pool->submit(produceValue);
		auto fut2 = fut.then(intToStr);
		std::string result = *fut2;
		if(result != "6") {
			std::cerr << "FAIL: then int->string should give \"6\", got \"" << result << "\"\n";
			return -1;
		}
	}

	//---- multi-step chain: int -> int -> string ----
	{
		auto fut = pool->submit(produceValue);
		auto fut2 = fut.then(doubleValue);
		auto fut3 = fut2.then(intToStr);
		std::string result = *fut3;
		if(result != "12") {
			std::cerr << "FAIL: 3-step chain should give \"12\", got \"" << result << "\"\n";
			return -1;
		}
	}

	//---- thenDetached: void continuation ----
	{
		auto fut = pool->submit(produceValue);
		fut.thenDetached(consumeInt);
		pool->waitIdle();
	}

	//---- then() on an already-complete future ----
	{
		auto fut = pool->submit(produceValue);
		fut.await();
		auto fut2 = fut.then(doubleValue);
		int result = *fut2;
		if(result != 12) {
			std::cerr << "FAIL: then() on already-complete future should give 12, got " << result << "\n";
			return -1;
		}
	}

	std::cout << "PASS\n";
	return 0;
}
