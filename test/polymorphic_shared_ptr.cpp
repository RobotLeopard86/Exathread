#include "exathread.hpp"
#include <atomic>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

//---- Polymorphic hierarchy ----

static std::atomic<int> g_liveCount {0};

struct Base {
	int tag = 0;
	std::string label = "base";

	Base() {
		++g_liveCount;
	}
	virtual ~Base() {
		--g_liveCount;
	}
	virtual int id() const {
		return 0;
	}
};

struct Derived : public Base {
	std::string extra = "derived-payload";

	Derived() {
		tag = 42;
		label = "derived";
	}
	int id() const override {
		return 1;
	}
};

struct Unrelated : public Base {
	Unrelated() {
		tag = -1;
		label = "unrelated";
	}
	int id() const override {
		return 2;
	}
};

//---- Inner coroutine ----
//Builds a shared_ptr<Base> (actually pointing at a Derived) and returns it.
//Does NOT co_await anything internally on this path, matching the "cache miss,
//no internal suspension" branch described as the one actually exercised.
static exathread::ValueTask<std::shared_ptr<Base>> inner(int which) {
	std::shared_ptr<Base> result;
	if(which % 3 == 0) {
		result = std::make_shared<Unrelated>();
	} else {
		result = std::make_shared<Derived>();
	}
	co_return result;
}

//---- Outer coroutine ----
static exathread::ValueTask<std::shared_ptr<Derived>> outer(
	std::shared_ptr<exathread::Pool> pool, int which) {
	exathread::Future<std::shared_ptr<Base>> fut = pool->submit(inner, which);
	co_await exathread::yieldUntilComplete(fut);
	std::shared_ptr<Base> result = *fut;

	if(!result) {
		std::cerr << "FAIL: result is null for which=" << which << "\n";
		co_return nullptr;
	}

	//This is the line that crashes for the user: dynamic_pointer_cast on a
	//shared_ptr whose control block may already be corrupted.
	std::shared_ptr<Derived> casted = std::dynamic_pointer_cast<Derived>(result);
	co_return casted;
}

int main() {
	auto pool = exathread::Pool::Create(4);

	constexpr int kIterations = 500;
	int derivedHits = 0;
	int unrelatedMisses = 0;

	for(int i = 0; i < kIterations; ++i) {
		auto fut = pool->submit(outer, pool, i);
		fut.await();
		std::shared_ptr<Derived> d = *fut;

		if(i % 3 == 0) {
			//Should be null: source was Unrelated, cast should fail cleanly.
			if(d != nullptr) {
				std::cerr << "FAIL: expected null cast at i=" << i << " but got non-null\n";
				return -1;
			}
			++unrelatedMisses;
		} else {
			if(d == nullptr) {
				std::cerr << "FAIL: expected valid Derived at i=" << i << " but got null\n";
				return -1;
			}
			if(d->tag != 42 || d->extra != "derived-payload" || d->id() != 1) {
				std::cerr << "FAIL: corrupted Derived data at i=" << i
						  << " tag=" << d->tag << " extra=\"" << d->extra
						  << "\" id()=" << d->id() << "\n";
				return -1;
			}
			++derivedHits;
		}
	}

	pool->waitIdle();
	pool.reset();

	if(g_liveCount.load() != 0) {
		std::cerr << "FAIL: leaked " << g_liveCount.load() << " Base-derived objects\n";
		return -1;
	}

	std::cout << "PASS (" << derivedHits << " derived hits, " << unrelatedMisses
			  << " unrelated misses)\n";
	return 0;
}