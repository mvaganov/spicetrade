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
	VList<PlayAction*> unplayedCards;
	VList<PlayAction*> playedCards;
	VList<Objective*> objectives;
	RBTree<ResourceType*, int> resourceInventory;
	List<char> resourceStore;
};