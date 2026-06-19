//Test 01: Pool creation and thread count
//Verifies Pool::Create(), getThreadCount(), and the thread-count limit guard.

#include "exathread.hpp"

#include <cstdlib>
#include <iostream>

int main() {
	const std::size_t HW = std::thread::hardware_concurrency();

	//---- Basic creation ----
	{
		auto pool = exathread::Pool::Create(2);
		if(!pool) {
			std::cerr << "FAIL: Pool::Create returned null\n";
			return -1;
		}
		if(pool->getThreadCount() != 2) {
			std::cerr << "FAIL: expected 2 threads, got " << pool->getThreadCount() << "\n";
			return -1;
		}
	}
	//Pool destroyed here; totalThreads should be back to 0.

	//---- Default thread count (hardware_concurrency / 2) ----
	{
		std::size_t expected = HW / 2;
		auto pool = exathread::Pool::Create();
		if(pool->getThreadCount() != expected) {
			std::cerr << "FAIL: default thread count: expected " << expected
					  << ", got " << pool->getThreadCount() << "\n";
			return -1;
		}
	}

	//---- Over-limit guard ----
	{
		bool threw = false;
		try {
			auto pool = exathread::Pool::Create(HW + 1);
		} catch(const std::out_of_range&) {
			threw = true;
		} catch(...) {
			std::cerr << "FAIL: wrong exception type thrown for over-limit pool\n";
			return -1;
		}
		if(!threw) {
			std::cerr << "FAIL: no exception thrown for over-limit pool\n";
			return -1;
		}
	}

	std::cout << "PASS\n";
	return 0;
}
