#pragma once
struct Objective {
	int points;
	int bonusPoints; // for the gold or silver coin
	int bonusColor;
	const char* input;
	const char* name;
	Objective (const char* input, int points, const char* name)
		: input (input), points (points), bonusPoints (0), name (name) {}
};