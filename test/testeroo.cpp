#include "../include/exathread.hpp"

#include <cassert>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

std::string reado(std::string fp) {
	std::ifstream ifs(fp);
	std::stringstream ss;
	ss << ifs.rdbuf();
	std::stringstream writemsg;
	writemsg << "Read \"" << ss.str() << "\"" << std::endl;
	std::cout << writemsg.str();
	return ss.str();
}

void printo(std::vector<std::string> results) {
	std::cout << "------" << std::endl;
	for(const std::string& s : results) {
		std::cout << "Got \"" << s << "\"" << std::endl;
	}
}

int main() {
	std::shared_ptr<exathread::Pool> pool = exathread::Pool::Create();
	std::vector<std::string> files = {
		"a.txt", "b.txt", "c.txt", "d.txt", "e.txt"};
	auto job = pool->batch(files, reado);
	job.then(printo).await();
	auto results = job.results();
	std::map<std::string, bool> check = {
		{"A is for Apple", false},
		{"B is for Banana", false},
		{"C is for Coconut", false},
		{"D is for Durian (stinky)", false},
		{"E is for Elephant (I know it's not a fruit but what are you going to do about it)", false}};
	for(const std::string& str : results) {
		if(check.contains(str)) check.at(str) = true;
		try {
			std::string r = (check.at(str) ? "yup" : "nah");
			std::cout << str << ": " << r << std::endl;
		} catch(...) {}
	}
	std::cout << "------" << std::endl;
	for(const auto& [s, v] : check) {
		//assert(v && "Test failed; not all strings found in result array!");
		std::cout << s << ": " << (v ? "yup" : "nah") << std::endl;
	}
}