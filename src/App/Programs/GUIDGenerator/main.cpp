#include <_deps/crossguid/guid.hpp>

#include <iostream>

int main() {
	std::cout << xg::newGuid().str() << std::endl;
	return 0;
}
