#include "../include/exathread.hpp"

#include <memory>
#include <iostream>
#include <fstream>

std::string reado(const std::string& fp) {
	std::ifstream ifs(fp);
	std::stringstream ss;
	ss << ifs.rdbuf();
	return ss.str();
}

void printo(std::vector<std::string> results) {
	for(const std::string& s : results) {
		std::cout << s << std::endl;
	}
}

int main() {
	std::shared_ptr<exathread::Pool> pool = exathread::Pool::Create();
	std::vector<std::string> files = {
		"a.txt", "b.txt", "c.txt", "d.txt", "e.txt"};
	pool->batch(files, reado).then(printo).await();
}