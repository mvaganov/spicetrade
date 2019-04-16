#pragma once
struct Objective {
	int points;
	int bonusPoints; // for the gold or silver coin
	int bonusColor;
	const char* resourceCost;
	const char* name;
	Objective (const char* resourceCost, int points, const char* name)
		: resourceCost (resourceCost), points (points), bonusPoints (0), name (name) {}
};