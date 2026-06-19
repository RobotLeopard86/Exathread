//Test 03: submit() with a value-returning function; dereference Future to get result.

#include "exathread.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

static int add(int a, int b) {
	return a + b;
}

static std::string makeString(int x) {
	return "value=" + std::to_string(x);
}

int main() {
	auto pool = exathread::Pool::Create(2);

	//---- int result ----
	{
		auto fut = pool->submit(add, 3, 4);
		int result = *fut;//operator* blocks until done
		if(result != 7) {
			std::cerr << "FAIL: add(3,4) should return 7, got " << result << "\n";
			return -1;
		}
		if(fut.checkStatus() != exathread::Status::Complete) {
			std::cerr << "FAIL: future status should be Complete after dereference\n";
			return -1;
		}
	}

	//---- std::string result ----
	{
		auto fut = pool->submit(makeString, 42);
		std::string result = *fut;
		if(result != "value=42") {
			std::cerr << "FAIL: makeString(42) should return \"value=42\", got \"" << result << "\"\n";
			return -1;
		}
	}

	//---- operator-> ----
	{
		auto fut = pool->submit(makeString, 7);
		std::size_t len = fut->size();//operator-> on Future<std::string>
		std::string expected = "value=7";
		if(len != expected.size()) {
			std::cerr << "FAIL: operator-> returned wrong size: " << len << "\n";
			return -1;
		}
	}

	std::cout << "PASS\n";
	return 0;
}
