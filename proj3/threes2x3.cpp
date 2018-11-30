/**
 * Basic Environment for Game threes2x3
 * use 'g++ -std=c++11 -O3 -g -o threes2x3 threes2x3.cpp' to compile the source
 *
 * Computer Games and Intelligence (CGI) Lab, NCTU, Taiwan
 * http://www.aigames.nctu.edu.tw
 */

#include <iostream>
#include <fstream>
#include <iterator>
#include <string>
#include "board2x3.h"
#include "solver.h"

int main(int argc, const char* argv[]) {
	std::cout << "threes2x3-Demo: " << std::endl;
	solver solve;
	board2x3 state;
	state_type type;
	state_hint hint(state);
	
	std::cout << std::fixed << std::setprecision(3);
	while (std::cin >> type >> state >> hint) {
		auto value = solve.solve(state, type);
		std::cout << type << " " << state << hint;
		std::cout << " = " << value << std::endl;
	}

	return 0;
}
