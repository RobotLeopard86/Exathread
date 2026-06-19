//Test 16: VoidPromise continuation scheduling.
//Verifies that completing a void task fires all tasks in its `next` list,
//covering VoidPromise::continuation() and the various void scheduling paths.

#include "exathread.hpp"

#include <atomic>
#include <cstdlib>
#include <iostream>
#include <vector>

static std::atomic<int> counter {0};
static void increment() {
	++counter;
}
static void addThree() {
	counter += 3;
}
static void noop() {}
static int produceVal() {
	return 99;
}

int main() {
	auto pool = exathread::Pool::Create(2);

	//---- Single void -> void continuation via thenDetached ----
	{
		counter = 0;
		auto fut = pool->submit(noop);
		fut.thenDetached(increment);
		pool->waitIdle();
		auto dl = std::chrono::steady_clock::now() + std::chrono::seconds(5);
		while(counter.load() != 1 && std::chrono::steady_clock::now() < dl)
			std::this_thread::yield();
		if(counter.load() != 1) {
			std::cerr << "FAIL: single void->void thenDetached: counter=" << counter.load() << "\n";
			return -1;
		}
	}

	//---- Multiple continuations on the same void future ----
	//Attaching two thenDetached continuations to the same future should schedule both.
	{
		counter = 0;
		auto fut = pool->submit(noop);
		fut.thenDetached(increment);
		fut.thenDetached(addThree);
		pool->waitIdle();
		auto dl = std::chrono::steady_clock::now() + std::chrono::seconds(5);
		while(counter.load() != 4 && std::chrono::steady_clock::now() < dl)
			std::this_thread::yield();
		if(counter.load() != 4) {
			std::cerr << "FAIL: two thenDetached on same future: counter=" << counter.load() << " (expected 4)\n";
			return -1;
		}
	}

	//---- void -> int then (VoidPromise fires a ValueTask continuation) ----
	{

		auto fut = pool->submit(noop);
		auto intFut = fut.then(produceVal);
		if(*intFut != 99) {
			std::cerr << "FAIL: void->int then: got " << *intFut << "\n";
			return -1;
		}
	}

	//---- void -> void thenBatch (Future<void>::thenBatch) ----
	{
		counter = 0;
		std::vector<int> items = {1, 2, 3, 4, 5};
		auto fut = pool->submit(noop);
		auto mfut = fut.thenBatch(items, [](int x) { counter += x; });
		mfut.await();
		//sum = 15
		if(counter.load() != 15) {
			std::cerr << "FAIL: void thenBatch: counter=" << counter.load() << " (expected 15)\n";
			return -1;
		}
	}

	//---- void thenBatchDetached ----
	{
		counter = 0;
		std::vector<int> items = {10, 20, 30};
		auto fut = pool->submit(noop);
		fut.thenBatchDetached(items, [](int x) { counter += x; });
		pool->waitIdle();
		auto dl = std::chrono::steady_clock::now() + std::chrono::seconds(5);
		while(counter.load() != 60 && std::chrono::steady_clock::now() < dl)
			std::this_thread::yield();
		if(counter.load() != 60) {
			std::cerr << "FAIL: void thenBatchDetached: counter=" << counter.load() << " (expected 60)\n";
			return -1;
		}
	}

	std::cout << "PASS\n";
	return 0;
}
