//Test 19: MultiFuture initializer_list construction, size(), and in-progress status.

#include "exathread.hpp"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <vector>

static std::atomic<bool> gate {false};

//Blocks until the gate is opened so we can observe in-progress MultiFuture status.
static exathread::VoidTask gated(std::atomic<bool>* g) {
	co_await exathread::yieldUntilTrue([g]() { return g->load(); });
}

static int identity(int x) {
	return x;
}

int main() {
	auto pool = exathread::Pool::Create(2);

	//---- initializer_list construction with Future<int> ----
	{
		auto f1 = pool->submit(identity, 1);
		auto f2 = pool->submit(identity, 2);
		auto f3 = pool->submit(identity, 3);
		//Initializer-list constructor (non-explicit for T != void based on declaration)
		exathread::MultiFuture<int> mfut {f1, f2, f3};
		if(mfut.size() != 3) {
			std::cerr << "FAIL: initializer_list MultiFuture<int> size should be 3, got " << mfut.size() << "\n";
			return -1;
		}
		auto results = mfut.results();
		//Results are in submission order
		if(results[0] != 1 || results[1] != 2 || results[2] != 3) {
			std::cerr << "FAIL: initializer_list results wrong: "
					  << results[0] << " " << results[1] << " " << results[2] << "\n";
			return -1;
		}
	}

	//---- initializer_list construction with Future<void> ----
	{
		auto f1 = pool->submit([]() {});
		auto f2 = pool->submit([]() {});
		exathread::MultiFuture<void> mfut {f1, f2};
		if(mfut.size() != 2) {
			std::cerr << "FAIL: initializer_list MultiFuture<void> size should be 2, got " << mfut.size() << "\n";
			return -1;
		}
		mfut.await();
		if(mfut.checkStatus() != exathread::Status::Complete) {
			std::cerr << "FAIL: MultiFuture<void> should be Complete\n";
			return -1;
		}
	}

	//---- Not-all-done: at least one task still running -> status not Complete ----
	{
		gate = false;
		//Submit one gated (blocking) task and one fast task
		auto slowFut = pool->submit(gated, &gate);
		auto fastFut = pool->submit(identity, 99);
		fastFut.await();//fast one done

		exathread::MultiFuture<void> mfut {slowFut};
		//Give the slow task a moment to start and yield
		std::this_thread::sleep_for(std::chrono::milliseconds(20));

		exathread::Status s = mfut.checkStatus();
		if(s == exathread::Status::Complete || s == exathread::Status::Failed) {
			std::cerr << "FAIL: MultiFuture should not be done while a task is still gated, got status "
					  << static_cast<int>(s) << "\n";
			gate = true;//unblock before returning
			return -1;
		}

		//Now unblock and wait
		gate = true;
		mfut.await();
		if(mfut.checkStatus() != exathread::Status::Complete) {
			std::cerr << "FAIL: MultiFuture should be Complete after gate opened\n";
			return -1;
		}
	}

	std::cout << "PASS\n";
	return 0;
}
