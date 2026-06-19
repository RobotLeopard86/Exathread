//Test 21: Ring buffer stress test focusing on head/tail pointer races.
//We hammer the pool with enough tasks to wrap the ring buffer (capacity 4096)
//multiple times, using multiple producer threads to stress the CAS loops in
//push() and pop(). All results must be correct and no tasks may be lost.

#include "exathread.hpp"

#include <atomic>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>

static int square(int x) {
	return x * x;
}

int main() {
	auto pool = exathread::Pool::Create(4);

	//---- Wrap the ring buffer multiple times ----
	//4096 capacity; submit 12288 tasks (3 full wraps).
	{
		constexpr int N = 12288;
		std::vector<exathread::Future<int>> futs;
		futs.reserve(N);

		//Submit in batches to avoid filling the ring buffer before workers drain it
		constexpr int BATCH = 512;
		for(int base = 0; base < N; base += BATCH) {
			for(int i = base; i < base + BATCH; ++i)
				futs.push_back(pool->submit(square, i));
			//Let workers make some progress before the next batch
			std::this_thread::yield();
		}

		for(int i = 0; i < N; ++i) {
			int result = *futs[i];
			if(result != i * i) {
				std::cerr << "FAIL: square(" << i << ") = " << result
						  << ", expected " << i * i << "\n";
				return -1;
			}
		}
	}

	//---- Concurrent producers: multiple threads submitting simultaneously ----
	{
		constexpr int PRODUCERS = 4;
		constexpr int PER_PRODUCER = 256;

		//Each producer submits its own slice; results indexed by producer*PER_PRODUCER+i
		std::vector<std::vector<exathread::Future<int>>> allFuts(PRODUCERS);
		std::vector<std::thread> producers;
		producers.reserve(PRODUCERS);

		for(int p = 0; p < PRODUCERS; ++p) {
			allFuts[p].reserve(PER_PRODUCER);
			producers.emplace_back([&, p]() {
				for(int i = 0; i < PER_PRODUCER; ++i)
					allFuts[p].push_back(pool->submit(square, p * PER_PRODUCER + i));
			});
		}
		for(auto& t : producers) t.join();

		for(int p = 0; p < PRODUCERS; ++p) {
			for(int i = 0; i < PER_PRODUCER; ++i) {
				int val = p * PER_PRODUCER + i;
				int result = *allFuts[p][i];
				if(result != val * val) {
					std::cerr << "FAIL: concurrent producer: square(" << val << ") = "
							  << result << ", expected " << val * val << "\n";
					return -1;
				}
			}
		}
	}

	//---- Interleaved submit/await: ensures pop() head advancement is correct ----
	{
		constexpr int N = 1000;
		for(int i = 0; i < N; ++i) {
			auto fut = pool->submit(square, i);
			int result = *fut;//await immediately
			if(result != i * i) {
				std::cerr << "FAIL: interleaved: square(" << i << ") = "
						  << result << ", expected " << i * i << "\n";
				return -1;
			}
		}
	}

	std::cout << "PASS\n";
	return 0;
}
