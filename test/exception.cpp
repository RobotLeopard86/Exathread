//Test 05: If the submitted task throws, the exception is stored and re-thrown
//when the caller dereferences the Future.

#include "exathread.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>

static int thrower(int x) {
	if(x < 0)
		throw std::runtime_error("negative input");
	return x * 2;
}

static void voidThrower() {
	throw std::logic_error("voidThrower fired");
}

int main() {
	auto pool = exathread::Pool::Create(2);

	//---- Value future: exception on dereference ----
	{
		auto fut = pool->submit(thrower, -1);
		fut.await();

		if(fut.checkStatus() != exathread::Status::Failed) {
			std::cerr << "FAIL: status should be Failed after exception, got "
					  << static_cast<int>(fut.checkStatus()) << "\n";
			return -1;
		}

		bool caught = false;
		try {
			int val = *fut;
			(void)val;
		} catch(const std::runtime_error& e) {
			caught = true;
			std::string msg = e.what();
			if(msg != "negative input") {
				std::cerr << "FAIL: wrong exception message: " << msg << "\n";
				return -1;
			}
		} catch(...) {
			std::cerr << "FAIL: wrong exception type\n";
			return -1;
		}
		if(!caught) {
			std::cerr << "FAIL: no exception thrown on dereference of failed future\n";
			return -1;
		}
	}

	//---- Successful case still works after an exception test ----
	{
		auto fut = pool->submit(thrower, 5);
		if(*fut != 10) {
			std::cerr << "FAIL: thrower(5) should return 10\n";
			return -1;
		}
	}

	//---- Void future: exception recorded as Failed status ----
	{
		auto fut = pool->submit(voidThrower);
		fut.await();
		if(fut.checkStatus() != exathread::Status::Failed) {
			std::cerr << "FAIL: void future status should be Failed\n";
			return -1;
		}
	}

	std::cout << "PASS\n";
	return 0;
}
