#pragma once
#include "list.h"
#include "objective.h"
#include "rbtree.h"
#include "resource.h"
#include "tradecard.h"
#include <string>
#include "dictionary.h"

#define MAX_RESOURCES 10

enum PredictionState {none, valid, invalid};
enum UserControl {ui_hand, ui_inventory, ui_cards, ui_acquire, ui_objectives, ui_upgrade};

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
	Dictionary<int, int> selectedMark;
	VList<int> selected; // TODO rename selectedCards
	PredictionState validPrediction;
	UserControl ui, lastState;


	int inventoryCursor;
	int handOffset;
	int currentRow; // TODO rename handCursor
	int marketCursor;
	int marketCardToBuy; // TODO rename market choice
	int upgradeChoices = 0; // TODO rename upgradeChoicesToMake
	int upgradesMade = 0;

	Player():validPrediction(PredictionState::none),ui(UserControl::ui_hand),
		inventoryCursor(0),handOffset(0),currentRow(0),marketCursor(0),
		marketCardToBuy(0),upgradeChoices(0),upgradesMade(0){}

	void Set(std::string name, int resourceCount) { 
		this->name = name;
		inventory.SetLength(resourceCount);
		inventory.SetAll(0);
		inventoryPrediction.Copy(inventory);
	}
	void Add(const PlayAction* actions, const int count) {
		for(int i = 0; i < count; ++i) {
			hand.Add (&(actions[i]));
		}
	}
	void Draw(VList<const PlayAction*>& ref_deck, int count = 1) {
		for (int i = 0; i < count; ++i) {
			hand.Add (ref_deck.PopLast ());
		}
	}
	void PredictionRestart() {
		handPrediction.Copy(hand);
		playedPrediction.Copy(played);
	}
};