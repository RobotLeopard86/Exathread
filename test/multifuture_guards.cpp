//Test 13: MultiFuture construction error conditions.
//- Empty list should throw std::length_error.
//- Futures from different pools should throw std::logic_error.

#include "exathread.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>

static void noop() {}

int main() {
	//---- Empty initializer_list ----
	{
		bool threw = false;
		try {
			exathread::MultiFuture<void> mf({});
		} catch(const std::length_error&) {
			threw = true;
		} catch(...) {
			std::cerr << "FAIL: wrong exception for empty MultiFuture\n";
			return -1;
		}
		if(!threw) {
			std::cerr << "FAIL: no exception for empty MultiFuture\n";
			return -1;
		}
	}

	//---- Futures from different pools ----
	{
		auto pool1 = exathread::Pool::Create(2);
		auto pool2 = exathread::Pool::Create(2);

		auto f1 = pool1->submit(noop);
		auto f2 = pool2->submit(noop);
		f1.await();
		f2.await();

		bool threw = false;
		try {
			exathread::MultiFuture<void> mf({f1, f2});
		} catch(const std::logic_error&) {
			threw = true;
		} catch(...) {
			std::cerr << "FAIL: wrong exception for cross-pool MultiFuture\n";
			return -1;
		}
		if(!threw) {
			std::cerr << "FAIL: no exception for cross-pool MultiFuture\n";
			return -1;
		}
	}

	std::cout << "PASS\n";
	return 0;
}
