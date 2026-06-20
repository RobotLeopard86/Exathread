//Test 17: Task handle lifetime — verifies that the coroutine frame stays alive
//as long as any Future referencing it exists, and that copy/move semantics
//of Future correctly extend or transfer that lifetime.

#include "exathread.hpp"

#include <cstdlib>
#include <iostream>

static int identity(int x) {
	return x;
}
static void noop() {}

int main() {
	auto pool = exathread::Pool::Create(2);

	//---- Copy: both futures see the same result ----
	//Copying a Future should share the underlying frame; both must be Complete
	//and return the same value after one of them is awaited.
	{
		auto fut1 = pool->submit(identity, 7);
		exathread::Future<int> fut2 = fut1;//copy
		fut1.await();
		//fut2 shares the frame, so it should also see Complete and the same value
		if(fut2.checkStatus() != exathread::Status::Complete) {
			std::cerr << "FAIL: copied future should see Complete status\n";
			return -1;
		}
		if(*fut2 != 7) {
			std::cerr << "FAIL: copied future should return 7, got " << *fut2 << "\n";
			return -1;
		}
	}

	//---- Move: moved-from future is unusable, moved-to future is valid ----
	//After std::move(fut1), fut1's internal Task has a null handle/promise.
	//Calling checkStatus()/operator* on a moved-from Future is not supported
	//(promise() throws "No associated promise!" by design), so we don't touch
	//fut1 again — we only verify the moved-to future works correctly.
	{
		auto fut1 = pool->submit(identity, 42);
		exathread::Future<int> fut2 = std::move(fut1);
		if(*fut2 != 42) {
			std::cerr << "FAIL: moved-to future should return 42, got " << *fut2 << "\n";
			return -1;
		}
	}

	//---- Frame lifetime: future kept alive across scope boundary ----
	//Submit a task, let the original Future go out of scope while a copy survives.
	//The frame must not be destroyed until the copy is also gone.
	{
		auto fut = pool->submit(identity, 99);
		exathread::Future<int> keeper = fut;//copy — refcount now 2
		{
			auto inner = fut;//another copy, immediately discarded at end of this scope
			(void)inner;
		}
		//keeper still holds a reference; frame must still be valid
		if(*keeper != 99) {
			std::cerr << "FAIL: frame destroyed too early; keeper should return 99\n";
			return -1;
		}
	}

	//---- Multiple copies all see completion ----
	{
		auto fut1 = pool->submit(identity, 5);
		auto fut2 = fut1;
		auto fut3 = fut2;
		fut1.await();
		if(*fut2 != 5 || *fut3 != 5) {
			std::cerr << "FAIL: all copies should return 5\n";
			return -1;
		}
		if(fut2.checkStatus() != exathread::Status::Complete ||
			fut3.checkStatus() != exathread::Status::Complete) {
			std::cerr << "FAIL: all copies should be Complete\n";
			return -1;
		}
	}

	//---- Void future copy/move ----
	{
		auto fut1 = pool->submit(noop);
		exathread::Future<void> fut2 = fut1;
		fut1.await();
		if(fut2.checkStatus() != exathread::Status::Complete) {
			std::cerr << "FAIL: copied void future should be Complete\n";
			return -1;
		}
	}
	{
		auto fut1 = pool->submit(noop);
		auto fut2 = std::move(fut1);
		fut2.await();
		if(fut2.checkStatus() != exathread::Status::Complete) {
			std::cerr << "FAIL: moved-to void future should be Complete\n";
			return -1;
		}
	}

	std::cout << "PASS\n";
	return 0;
}