#include "list.h"
#include "objective.h"
#include "rbtree.h"
#include "resource.h"
#include "tradecard.h"
#include <string>

#define MAX_RESOURCES 10

class Player {
  public:
	std::string name;
	VList<Objective*> objectives;

	VList<PlayAction*> hand;
	VList<PlayAction*> played;
	List<int> inventory;

	VList<PlayAction*> handPrediction;
	VList<PlayAction*> playedPrediction;
	List<int> inventoryPrediction;

};