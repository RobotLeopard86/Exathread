//Test 06: Pool::batch() dispatches one task per input element;
//MultiFuture::results() collects them in submission order.
//MultiFuture::size() returns the number of futures.

#include "exathread.hpp"

#include <cstdlib>
#include <iostream>
#include <vector>

static int doubleIt(int x) {
	return x * 2;
}

int main() {
	auto pool = exathread::Pool::Create(2);

	//---- batch returning values ----
	{
		std::vector<int> input = {1, 2, 3, 4, 5};
		auto mfut = pool->batch(input, doubleIt);

		auto results = mfut.results();
		if(results.size() != input.size()) {
			std::cerr << "FAIL: results size mismatch: " << results.size()
					  << " vs " << input.size() << "\n";
			return -1;
		}
		for(std::size_t i = 0; i < input.size(); ++i) {
			if(results[i] != input[i] * 2) {
				std::cerr << "FAIL: results[" << i << "] = " << results[i]
						  << ", expected " << input[i] * 2 << "\n";
				return -1;
			}
		}
	}

	//---- MultiFuture::size() ----
	{
		std::vector<int> input = {10, 20, 30};
		auto mfut = pool->batch(input, doubleIt);
		if(mfut.size() != 3) {
			std::cerr << "FAIL: MultiFuture::size() returned " << mfut.size()
					  << ", expected 3\n";
			return -1;
		}
		mfut.await();
	}

	//---- batchDetached: verify it runs without crashing ----
	{
		std::vector<int> input = {0, 1, 2, 3};
		pool->batchDetached(input, [](int x) { (void)x; });
		pool->waitIdle();
	}

	std::cout << "PASS\n";
	return 0;
}
