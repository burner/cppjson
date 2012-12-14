#include <iostream>
#include "cppjson.hpp"
#include "unit.hpp"

int main() {
	if(!Unit::runTests()) {
		std::cout<<"found error exiting program"<<std::endl;
		return 1;
	}
	auto t = cppjson::parseFile("hull.js");
	t.print(std::cout);

	return 0;
}
