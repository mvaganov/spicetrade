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
	VList<Objective> achieved;

	VList<const PlayAction*> hand;
	VList<const PlayAction*> played;
	List<int> inventory;

	VList<const PlayAction*> handPrediction;
	VList<const PlayAction*> playedPrediction;
	List<int> inventoryPrediction;

	void Set(std::string name) { this->name = name; }
	void Add(const PlayAction* actions, int count) {
		for(int i = 0; i < count; ++i) {
			hand.Add (&(actions[i]));
		}
	}
	void Draw(VList<const PlayAction*> deck, int count = 1) {
		for (int i = 0; i < count; ++i) {
			hand.Add (deck.PopLast ());
		}
	}
	void PredictionRestart() {
		handPrediction.Copy(hand);
		playedPrediction.Copy(played);
	}
};