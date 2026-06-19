//Test 22: Exhaustive coverage of all Future<T> and MultiFuture<T> scheduling
//functions not already covered by earlier tests:
//  Future<T>::then, thenDetached, thenBatch, thenBatchDetached
//  Future<void>::then, thenDetached, thenBatch, thenBatchDetached
//  MultiFuture<T>::then, thenDetached, thenBatch, thenBatchDetached
//  MultiFuture<void>::then, thenDetached

#include "exathread.hpp"

#include <atomic>
#include <iostream>
#include <numeric>
#include <vector>

static std::atomic<int> counter {0};

static int doubleIt(int x) {
	return x * 2;
}
static int addInts(int a, int b) {
	return a + b;
}
static void consumeInt(int x) {
	counter += x;
}
static int sumVec(std::vector<int> v) {
	return std::accumulate(v.begin(), v.end(), 0);
}
static void sumVecVoid(std::vector<int> v) {
	counter += std::accumulate(v.begin(), v.end(), 0);
}
static void noop() {}

int main() {
	auto pool = exathread::Pool::Create(2);

	//======== Future<T> scheduling ========

	//Future<int>::then -> Future<int>
	{
		auto fut = pool->submit(doubleIt, 5);
		auto fut2 = fut.then(doubleIt);
		if(*fut2 != 20) {
			std::cerr << "FAIL: Future<int>::then: expected 20, got " << *fut2 << "\n";
			return -1;
		}
	}

	//Future<int>::thenDetached (already in test_07 but re-verify here)
	{
		counter = 0;
		auto fut = pool->submit(doubleIt, 3);
		fut.thenDetached(consumeInt);
		pool->waitIdle();
		auto dl = std::chrono::steady_clock::now() + std::chrono::seconds(5);
		while(counter.load() != 6 && std::chrono::steady_clock::now() < dl)
			std::this_thread::yield();
		if(counter.load() != 6) {
			std::cerr << "FAIL: Future<int>::thenDetached: counter=" << counter.load() << "\n";
			return -1;
		}
	}

	//Future<int>::thenBatch -> MultiFuture<int>
	//The first arg to the batch function receives the prior result; second is the batch item.
	{
		auto fut = pool->submit(doubleIt, 10);//produces 20
		std::vector<int> items = {1, 2, 3};
		//func(prior: int, item: int) -> int
		auto mfut = fut.thenBatch(items, addInts);
		auto results = mfut.results();//{20+1, 20+2, 20+3} = {21, 22, 23}
		if(results.size() != 3 || results[0] != 21 || results[1] != 22 || results[2] != 23) {
			std::cerr << "FAIL: Future<int>::thenBatch: wrong results\n";
			return -1;
		}
	}

	//Future<int>::thenBatchDetached
	{
		counter = 0;
		auto fut = pool->submit(doubleIt, 5);//produces 10
		std::vector<int> items = {1, 2, 3};
		//func(prior: int, item: int) -> void: counter += prior + item
		fut.thenBatchDetached(items, [](int prior, int item) { counter += prior + item; });
		pool->waitIdle();
		//Each task gets (10+1), (10+2), (10+3); sum = 11+12+13 = 36
		auto dl = std::chrono::steady_clock::now() + std::chrono::seconds(5);
		while(counter.load() != 36 && std::chrono::steady_clock::now() < dl)
			std::this_thread::yield();
		if(counter.load() != 36) {
			std::cerr << "FAIL: Future<int>::thenBatchDetached: counter=" << counter.load() << "\n";
			return -1;
		}
	}

	//======== Future<void> scheduling ========

	//Future<void>::then -> Future<int>
	{
		auto fut = pool->submit(noop);
		auto fut2 = fut.then([]() { return 77; });
		if(*fut2 != 77) {
			std::cerr << "FAIL: Future<void>::then: expected 77, got " << *fut2 << "\n";
			return -1;
		}
	}

	//Future<void>::thenDetached
	{
		counter = 0;
		auto fut = pool->submit(noop);
		fut.thenDetached([]() { counter = 55; });
		pool->waitIdle();
		auto dl = std::chrono::steady_clock::now() + std::chrono::seconds(5);
		while(counter.load() != 55 && std::chrono::steady_clock::now() < dl)
			std::this_thread::yield();
		if(counter.load() != 55) {
			std::cerr << "FAIL: Future<void>::thenDetached: counter=" << counter.load() << "\n";
			return -1;
		}
	}

	//Future<void>::thenBatch -> MultiFuture<int>
	{
		auto fut = pool->submit(noop);
		std::vector<int> items = {4, 5, 6};
		auto mfut = fut.thenBatch(items, doubleIt);
		auto results = mfut.results();//{8, 10, 12}
		if(results.size() != 3 || results[0] != 8 || results[1] != 10 || results[2] != 12) {
			std::cerr << "FAIL: Future<void>::thenBatch: wrong results\n";
			return -1;
		}
	}

	//Future<void>::thenBatchDetached
	{
		counter = 0;
		auto fut = pool->submit(noop);
		std::vector<int> items = {7, 8, 9};
		fut.thenBatchDetached(items, consumeInt);
		pool->waitIdle();
		auto dl = std::chrono::steady_clock::now() + std::chrono::seconds(5);
		while(counter.load() != 24 && std::chrono::steady_clock::now() < dl)
			std::this_thread::yield();
		if(counter.load() != 24) {
			std::cerr << "FAIL: Future<void>::thenBatchDetached: counter=" << counter.load() << "\n";
			return -1;
		}
	}

	//======== MultiFuture<T> scheduling ========

	//MultiFuture<int>::then -> Future<int> (receives std::vector<int>)
	{
		std::vector<int> items = {1, 2, 3, 4, 5};
		auto mfut = pool->batch(items, doubleIt);//{2,4,6,8,10}
		auto fut = mfut.then(sumVec);			 //sum = 30
		if(*fut != 30) {
			std::cerr << "FAIL: MultiFuture<int>::then: expected 30, got " << *fut << "\n";
			return -1;
		}
	}

	//MultiFuture<int>::thenDetached (receives std::vector<int>)
	{
		counter = 0;
		std::vector<int> items = {1, 2, 3};
		auto mfut = pool->batch(items, doubleIt);//{2,4,6}
		mfut.thenDetached(sumVecVoid);			 //counter += 12
		pool->waitIdle();
		auto dl = std::chrono::steady_clock::now() + std::chrono::seconds(5);
		while(counter.load() != 12 && std::chrono::steady_clock::now() < dl)
			std::this_thread::yield();
		if(counter.load() != 12) {
			std::cerr << "FAIL: MultiFuture<int>::thenDetached: counter=" << counter.load() << "\n";
			return -1;
		}
	}

	//MultiFuture<int>::thenBatch -> MultiFuture<int>
	//Applies doubleIt to each result individually
	{
		std::vector<int> items = {1, 2, 3};//batch -> {2,4,6}
		auto mfut = pool->batch(items, doubleIt);
		auto mfut2 = mfut.thenBatch(doubleIt);//each doubled again -> {4,8,12}
		auto results = mfut2.results();
		if(results.size() != 3 || results[0] != 4 || results[1] != 8 || results[2] != 12) {
			std::cerr << "FAIL: MultiFuture<int>::thenBatch: wrong results\n";
			return -1;
		}
	}

	//MultiFuture<int>::thenBatchDetached
	{
		counter = 0;
		std::vector<int> items = {5, 10, 15};//batch -> {10, 20, 30}
		auto mfut = pool->batch(items, doubleIt);
		mfut.thenBatchDetached(consumeInt);//counter += 10+20+30 = 60
		pool->waitIdle();
		auto dl = std::chrono::steady_clock::now() + std::chrono::seconds(5);
		while(counter.load() != 60 && std::chrono::steady_clock::now() < dl)
			std::this_thread::yield();
		if(counter.load() != 60) {
			std::cerr << "FAIL: MultiFuture<int>::thenBatchDetached: counter=" << counter.load() << "\n";
			return -1;
		}
	}

	//======== MultiFuture<void> scheduling ========

	//MultiFuture<void>::then -> Future<int>
	{
		auto f1 = pool->submit(noop);
		auto f2 = pool->submit(noop);
		exathread::MultiFuture<void> mfut {f1, f2};
		auto fut = mfut.then([]() { return 123; });
		if(*fut != 123) {
			std::cerr << "FAIL: MultiFuture<void>::then: expected 123, got " << *fut << "\n";
			return -1;
		}
	}

	//MultiFuture<void>::thenDetached
	{
		counter = 0;
		auto f1 = pool->submit(noop);
		auto f2 = pool->submit(noop);
		exathread::MultiFuture<void> mfut {f1, f2};
		mfut.thenDetached([]() { counter = 99; });
		pool->waitIdle();
		auto dl = std::chrono::steady_clock::now() + std::chrono::seconds(5);
		while(counter.load() != 99 && std::chrono::steady_clock::now() < dl)
			std::this_thread::yield();
		if(counter.load() != 99) {
			std::cerr << "FAIL: MultiFuture<void>::thenDetached: counter=" << counter.load() << "\n";
			return -1;
		}
	}

	std::cout << "PASS\n";
	return 0;
}
