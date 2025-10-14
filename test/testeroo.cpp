#include "../include/exathread.hpp"

#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <chrono>

using namespace std::chrono_literals;

std::string reado(std::string fp) {
	std::ifstream ifs(fp);
	std::stringstream ss;
	ss << ifs.rdbuf();
	std::stringstream writemsg;
	writemsg << "Read \"" << ss.str() << "\" from file" << std::endl;
	std::cout << writemsg.str();
	return ss.str();
}


void printo(std::vector<std::string> results) {
	for(const std::string& s : results) {
		std::cout << "Got \"" << s << "\" in results list" << std::endl;
	}
}

exathread::VoidTask waito() {
	co_await exathread::yieldFor(1s);
}

int main() {
	std::shared_ptr<exathread::Pool> pool = exathread::Pool::Create();
	std::vector<std::string> files = {
		"a.txt", "b.txt", "c.txt", "d.txt", "e.txt", "f.txt", "g.txt"};
	auto job = pool->batch(files, reado);
	job.then(printo).await();
	auto results = job.results();
	std::map<std::string, bool> check = {
		{"A is for Apple", false},
		{"B is for Banana", false},
		{"C is for Coconut", false},
		{"D is for Durian (stinky)", false},
		{"E is for Eggplant (didn't know that was a fruit)", false},
		{"F is for Fig", false},
		{"G is for GRAPES", false}};
	for(const std::string& str : results) {
		if(check.contains(str)) check.at(str) = true;
	}
	for(const auto& [s, v] : check) {
		assert(v && "Test failed; not all strings found in result array!");
	}
	std::cout << "Yielding for 1 second... " << std::endl;
	pool->submit(waito).await();
	std::cout << "Done." << std::endl;
}
