#pragma once
#include <string>
struct Objective {
	int points;
	int bonusPoints; // for the gold or silver coin
	std::string input;
	std::string name;
	Objective():points(0),bonusPoints(0){}
	Objective (std::string input, int points, std::string name)
		: input (input), points (points), bonusPoints (0), name (name) {}
	Objective (const Objective & toCopy)
		: input (toCopy.input), points (toCopy.points), bonusPoints (0), name (toCopy.name) {}
};