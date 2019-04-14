#include "list.h"
#include "tradecard.h"
#include <string>
#include "objective.h"
#include "resource.h"
#include "rbtree.h"

#define MAX_RESOURCES   10

class Player {
public:
    std::string name;
    VList<TradeCard*> unplayedCards;
    VList<TradeCard*> playedCards;
    VList<Objective*> objectives;
    RBTree<ResourceType*, int> resourceInventory;
    List<char> resourceStore;
};