//Test 18: Exception rethrow via operator*, const operator*, and operator->,
//plus Failed task guards on then/thenDetached.

#include "exathread.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

static std::string makeStr(int x) {
	if(x < 0) throw std::runtime_error("negative");
	return "v" + std::to_string(x);
}

static int thrower() {
	throw std::logic_error("always fails");
	return 0;
}

static void voidThrower() {
	throw std::runtime_error("void failed");
}

static int identity(int x) {
	return x;
}

//Helper: run f on a const Future<T>& to exercise const operator*
template<typename T>
static T readConst(const exathread::Future<T>& f) {
	return *f;//const operator*
}

int main() {
	auto pool = exathread::Pool::Create(2);

	//---- operator* rethrows on Failed future ----
	{
		auto fut = pool->submit(thrower);
		fut.await();
		bool caught = false;
		try {
			(void)*fut;
		} catch(const std::logic_error&) { caught = true; }
		if(!caught) {
			std::cerr << "FAIL: operator* should rethrow on Failed future\n";
			return -1;
		}
	}

	//---- const operator* rethrows on Failed future ----
	{
		auto fut = pool->submit(thrower);
		fut.await();
		bool caught = false;
		try {
			(void)readConst(fut);
		} catch(const std::logic_error&) { caught = true; }
		if(!caught) {
			std::cerr << "FAIL: const operator* should rethrow on Failed future\n";
			return -1;
		}
	}

	//---- const operator* returns value on successful future ----
	{
		auto fut = pool->submit(makeStr, 3);
		std::string val = readConst(fut);
		if(val != "v3") {
			std::cerr << "FAIL: const operator* returned wrong value: " << val << "\n";
			return -1;
		}
	}

	//---- operator-> rethrows on Failed future ----
	{
		auto fut = pool->submit(makeStr, -1);
		fut.await();
		bool caught = false;
		try {
			(void)fut->size();
		} catch(const std::runtime_error&) { caught = true; }
		if(!caught) {
			std::cerr << "FAIL: operator-> should rethrow on Failed future\n";
			return -1;
		}
	}

	//---- const operator-> returns value ----
	{
		auto fut = pool->submit(makeStr, 7);
		//Access through a const ref to exercise const operator->
		const auto& cfut = fut;
		std::size_t len = cfut->size();
		if(len != std::string("v7").size()) {
			std::cerr << "FAIL: const operator-> returned wrong size: " << len << "\n";
			return -1;
		}
	}

	//---- then() on a Failed future throws std::logic_error ----
	{
		auto fut = pool->submit(thrower);
		fut.await();
		bool caught = false;
		try {
			(void)fut.then(identity);
		} catch(const std::logic_error&) { caught = true; }
		if(!caught) {
			std::cerr << "FAIL: then() on Failed future should throw logic_error\n";
			return -1;
		}
	}

	//---- thenDetached() on a Failed future throws std::logic_error ----
	{
		auto fut = pool->submit(voidThrower);
		fut.await();
		bool caught = false;
		try {
			fut.thenDetached([]() {});
		} catch(const std::logic_error&) { caught = true; }
		if(!caught) {
			std::cerr << "FAIL: thenDetached() on Failed future should throw logic_error\n";
			return -1;
		}
	}

	std::cout << "PASS\n";
	return 0;
}
