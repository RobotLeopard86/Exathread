//Test 15: MultiFuture::then() schedules a single continuation that receives
//all results as a std::vector<T>.

#include "exathread.hpp"

#include <cstdlib>
#include <iostream>
#include <numeric>
#include <vector>

static int doubleIt(int x) {
	return x * 2;
}

static int sumVec(std::vector<int> v) {
	return std::accumulate(v.begin(), v.end(), 0);
}

int main() {
	auto pool = exathread::Pool::Create(2);

	//Submit 5 tasks that double their input, then sum the results
	std::vector<int> inputs = {1, 2, 3, 4, 5};
	auto mfut = pool->batch(inputs, doubleIt);

	//Expected results: {2,4,6,8,10}, sum = 30
	auto sumFut = mfut.then(sumVec);
	int result = *sumFut;
	if(result != 30) {
		std::cerr << "FAIL: MultiFuture::then sum should be 30, got " << result << "\n";
		return -1;
	}

	std::cout << "PASS\n";
	return 0;
}
