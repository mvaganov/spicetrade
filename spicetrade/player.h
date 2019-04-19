#pragma once
#include "list.h"
#include "objective.h"
#include "rbtree.h"
#include "resource.h"
#include "playaction.h"
#include <string>
#include "dictionary.h"

#define MAX_RESOURCES 10

enum PredictionState {none, valid, invalid};
enum UserControl {ui_hand, ui_inventory, ui_cards, ui_acquire, ui_objectives, ui_upgrade};

class Game;

class Player {
  public:
	std::string name;
	std::string uimode;
	VList<Objective> achieved;

	VList<const PlayAction*> hand;
	VList<const PlayAction*> played;
	List<int> inventory;

	VList<const PlayAction*> handPrediction;
	VList<const PlayAction*> playedPrediction;
	List<int> inventoryPrediction;
	VList<int> selected; // TODO rename selectedCards
	Dictionary<int, int> selectedMark;
	PredictionState validPrediction;
	UserControl ui, lastState;


	int inventoryCursor;
	int handOffset;
	int currentRow; // TODO rename handCursor
	int marketCursor;
	int marketCardToBuy; // TODO rename market choice
	int upgradeChoices = 0; // TODO rename upgradeChoicesToMake
	int upgradesMade = 0;
	int fcolor, bcolor;

	Player():validPrediction(PredictionState::none),ui(UserControl::ui_hand),
		inventoryCursor(0),handOffset(0),currentRow(0),marketCursor(0),
		marketCardToBuy(0),upgradeChoices(0),upgradesMade(0),fcolor(-1),bcolor(-1){}

	void SetConsoleColor(Game& g)const;

	Player& Copy(const Player& toCopy) {
		#define listop(n)		n.Copy(toCopy.n);
		listop(achieved);	listop(hand);	listop(played);	listop(inventory);
		listop(handPrediction);	listop(playedPrediction);	listop(inventoryPrediction);
		listop(selected);
		#undef listop
		#define cpy(v)	v=toCopy.v;
		cpy(name);cpy(validPrediction);cpy(ui);cpy(lastState);
		cpy(inventoryCursor);cpy(handOffset);cpy(currentRow);
		cpy(marketCardToBuy);cpy(upgradeChoices);cpy(upgradesMade);
		cpy(fcolor);cpy(bcolor);
		#undef cpy
		return *this;
	}
	Player& Move(Player & toMove) {
		#define listop(n)		n.Move(toMove.n);
		listop(achieved);	listop(hand);	listop(played);	listop(inventory);
		listop(handPrediction);	listop(playedPrediction);	listop(inventoryPrediction);
		listop(selected);
		#undef listop
		#define cpy(v)	v=toMove.v;
		cpy(name);	cpy(validPrediction);	cpy(ui);cpy(lastState);
		cpy(inventoryCursor);cpy(handOffset);cpy(currentRow);
		cpy(marketCardToBuy);cpy(upgradeChoices);cpy(upgradesMade);
		#undef cpy
		return *this;
	}

	Player& operator=(const Player& toCopy) { return Copy(toCopy); }
	Player& operator=(Player&& toMove) { return Move(toMove); }

	void Set(std::string name, int resourceCount, int foregroundColor, int backgroundColor) { 
		this->name = name;
		fcolor = foregroundColor;
		bcolor = backgroundColor;
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

	// check if an action can be paid for with the given resources TODO rename CanAfford
	static bool CanPlay (Game& g, const PlayAction* toPlay, List<int>& inventory);

	static bool SubtractResources (Game& g, const PlayAction* card, List<int>& inventory);

	static bool InventoryValid(const List<int>& inventory);

	static void AddResources (Game& g, int& upgradesToDo, const PlayAction* card, List<int>& inventory, VList<const PlayAction*>& hand, VList<const PlayAction*>& played);

	static bool Calculate(Game& g, int& upgradesToDo, VList<const PlayAction*>& hand, VList<const PlayAction*>& played,
	VList<int>& selected, List<int>& prediction);

	static void RefreshPrediction(Game& g, Player& p);

	// event handling
	static void UpdateHand (Game& g, Player& p, int userInput, int count);
	static void UpdateInventory(Player& p, int userInput);
	static void UpdateUpgrade(Player& p, int userInput);
	static void UpdateInput(Player& p, Game& g, int move);

	// drawing UI
	static void PrintHand (Game& g, Coord pos, int count, Player& p);
	static void PrintInventory(const Player& p, Game& g, int background, int numberWidth, bool showZeros, List<int> & inventory, const VList<const ResourceType*>& collectableResources, Coord pos, int selected);
	static void PrintResourcesInventory(Coord cursor, Player& p, Game& g);
	static void PrintUserState(Coord cursor, const Player & p);
};