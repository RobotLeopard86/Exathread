//Test 14: Stress test — many concurrent tasks with a value-returning function,
//verifying correctness under real parallel load.

#include "exathread.hpp"

#include <cstdlib>
#include <iostream>
#include <numeric>
#include <vector>

static int square(int x) {
	return x * x;
}

int main() {
	auto pool = exathread::Pool::Create(4);

	constexpr int N = 200;
	std::vector<exathread::Future<int>> futs;
	futs.reserve(N);
	for(int i = 0; i < N; ++i)
		futs.push_back(pool->submit(square, i));

	//Collect and verify
	for(int i = 0; i < N; ++i) {
		int result = *futs[i];
		if(result != i * i) {
			std::cerr << "FAIL: square(" << i << ") = " << result
					  << ", expected " << i * i << "\n";
			return -1;
		}
	}

	std::cout << "PASS\n";
	return 0;
}
