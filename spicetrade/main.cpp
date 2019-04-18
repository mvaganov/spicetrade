#include "cli.h"
#include "dictionary.h"
#include "game.h"
#include "platform_conio.h"
#include "platform_random.h"
#include "vector2.h"
#include <stdio.h>
#include <string>

// 5 objective cards. +3 with gold token. +1 with silver
// 6 market cards
// each turn, 1 action
// * play a card (trade your resources)
//   - options rely on what inputs the cards need
//   - upgrade cubes can make X upgrades to cubes (which means they can be played on 1 cube to upgrade it X times). there should be a special cube-upgrader selection UI.
// * acquire a card from the market
//   - if the player wants the first card in the market, they get it (and any resources on it)
//   - if the player wants a later card, they put a resource on each card on the way there. UI to select which resource is placed on the market card
// * rest - pull cards back into your hand to be played again
// * purchase an objective with resources
// scoring: once the 6th objective card is purchased, finish that round, then score. every non-yellow cube is +1 point. objective card points are added together.

// required user states:
//x [play PlayAction from hand] - state_hand - select PlayAction to play (card)
//x [re-order PlayAction hand] - state_hand - during re-order, resource display changes to temp view
//x [acquire PlayAction] - state_acquire - select a card from the market. (gain resources on top of card if it has any)
//x [acquire deep PlayAction] - state_aquire_pay - pay for card if it isn't first first one.
//x [select Objective] - state_objective - purchase objective if resources are there.
//x [upgrade action] - state_upgrade - select X resources to upgrade
//x [rest action] - put played cards back into hand

// TODO cancel card purchase if enter is pressed twice without moving the inventory cursor

// architecture:
// game
//   action deck
//   achievement deck
// player
//   hand
//   inventory
//   achievements
//   _handPredicted
//   _inventoryPredicted

void print (std::string str) { CLI::printf ("%s", str.c_str ()); }
void print (const char* str) { CLI::printf ("%s", str); }
int clampint(int n, int mn, int mx) { return ((n<mn)?mn:((n>mx)?mx:n)); }

Dictionary<char, const ResourceType*> resourceLookup;
// code -> index-in-inventory
Dictionary<char, int> resourceIndex;

char ColorOfRes (char icon) {
	const ResourceType* r = resourceLookup.Get (icon);
	return (r == NULL) ? CLI::COLOR::LIGHT_GRAY : r->color;
}

void PrintAction (const PlayAction* a, int bg) {
	int ofcolor = CLI::getFcolor (), obcolor = CLI::getBcolor ();
	CLI::setColor (CLI::COLOR::LIGHT_GRAY, bg);

	const int width = 5;
	std::string str = a->input;
	int leadSpace = width - str.length ();
	for (int i = 0; i < leadSpace; ++i) { CLI::putchar (' '); }
	for (int i = 0; i < str.length (); ++i) {
		CLI::setColor (ColorOfRes (str[i]), bg);
		CLI::putchar (str[i]);
	}
	CLI::setColor (CLI::COLOR::LIGHT_GRAY, bg);
	CLI::putchar (':');
	str = a->output;
	leadSpace = width - str.length ();
	for (int i = 0; i < str.length (); ++i) {
		CLI::setColor (ColorOfRes (str[i]), bg);
		CLI::putchar (str[i]);
	}
	CLI::setColor (CLI::COLOR::LIGHT_GRAY, bg);
	for (int i = 0; i < leadSpace; ++i) { CLI::putchar (' '); }
	CLI::setColor (ofcolor, obcolor);
}

void PrintObjective (const Objective* o, int bg) {
	int ofcolor = CLI::getFcolor (), obcolor = CLI::getBcolor ();
	CLI::setColor (CLI::COLOR::LIGHT_GRAY, bg);

	const int width = 8;
	std::string str = o->input;
	int leadSpace = (width - str.length ())/2;
	for (int i = 0; i < leadSpace; ++i) { CLI::putchar (' '); }
	for (int i = 0; i < str.length (); ++i) {
		CLI::setColor (ColorOfRes (str[i]), bg);
		CLI::putchar (str[i]);
	}
	CLI::setColor (CLI::COLOR::LIGHT_GRAY, bg);
	// CLI::putchar (':');
	// str = a->output;
	leadSpace = width - str.length ();
	// for (int i = 0; i < str.length (); ++i) {
	// 	CLI::setColor (ColorOfRes (str[i]), bg);
	// 	CLI::putchar (str[i]);
	// }
	CLI::setColor (CLI::COLOR::LIGHT_GRAY, bg);
	for (int i = 0; i < leadSpace; ++i) { CLI::putchar (' '); }
	CLI::setColor (ofcolor, obcolor);
}

bool SubtractResources (const PlayAction* card, List<int>& inventory) {
	std::string input = card->input;
	if (input == "reset") {
		return true;
	}
	bool valid = true;
	for (int i = 0; i < input.length (); ++i) {
		char c = input[i];
		if (c == '.') {

		} else {
			int* index = resourceIndex.GetPtr (c);
			if (index == NULL) {
				printf ("can't afford imaginary resource %c\n", c);
				valid = false;
			}
			if(--inventory[*index] < 0) {
				valid = false;
			}
		}
	}
	return valid;
}

bool InventoryValid(const List<int>& inventory) {
	bool valid = true;
	for (int i = 0; i < inventory.Length (); ++i) {
		if (inventory[i] < 0) { valid = false; }
	}
	return valid;
}

bool CanPlay (const PlayAction* toPlay, List<int>& inventory) {
	List<int> testInventory (inventory); // TODO turn this into a static array and memcpy inventory?
	SubtractResources (toPlay, testInventory);
	return InventoryValid(testInventory);
}

void AddResources (int& upgradesToDo, const PlayAction* card, List<int>& inventory, VList<const PlayAction*>& hand, VList<const PlayAction*>& played) {
	std::string output = card->output;
	if (output == "cards") {
		hand.Insert (hand.Count (), played.GetData (), played.Count ());
		played.Clear ();
	} else {
		for (int i = 0; i < output.length (); ++i) {
			char c = output[i];
			if (c == '+') {
				upgradesToDo++;
			} else {
				int* index = resourceIndex.GetPtr (c);
				if (index == NULL) {
					printf ("no such resource %c\n", c);
				} // <-- here comes the crash
				inventory[*index]++;
			}
		}
	}
}

bool Calculate(int& upgradesToDo, VList<const PlayAction*>& hand, VList<const PlayAction*>& played,
	VList<int>& selected, List<int>& prediction) {
	bool canAfford = true;
	for(int i = 0; i < selected.Count(); ++i) {
		const PlayAction* card = hand[selected[i]];
		SubtractResources(card, prediction);
		AddResources(upgradesToDo, card, prediction, hand, played);
		bool canDoIt = InventoryValid(prediction);
		if(!canDoIt) { canAfford = false; }
	}
	return canAfford;
}

void RefreshPrediction(int& upgradesToDo, PredictionState& valid, VList<int>& selected, List<int>& inventory,
	List<int>& prediction, VList<const PlayAction*>& predictionHand, VList<const PlayAction*>& predictionPlayed){
	prediction.Copy(inventory);
	if(selected.Count() == 0){
		valid = PredictionState::none;
	} else {
		bool canAfford = Calculate(upgradesToDo, predictionHand, predictionPlayed, selected, prediction);
		valid = canAfford?PredictionState::valid:PredictionState::invalid;
	}
}

void UpdateObjectiveBuy(Player& p, int userInput, int& goldLeft, int& silverLeft, List<const Objective*>& achievements, VList<const Objective*>& achievement_deck){
	switch(userInput) {
	case 'a':
		p.marketCursor--; if(p.marketCursor < 0) { p.marketCursor = 0; }
		break;
	case 'd':
		p.marketCursor++; if(p.marketCursor >= achievements.Length()) { p.marketCursor = achievements.Length()-1; }
		break;
	case 's':
		p.ui = UserControl::ui_cards;
		break;
	case '\n': case '\r': {
		const Objective * o = achievements[p.marketCursor];
		// determine costs
		bool canAfford = true;
		p.inventoryPrediction.Copy(p.inventory);
		// check if the inventory has sufficient resources
		for(int i = 0; i < o->input.length(); ++i) {
			int resIndex = resourceIndex.Get(o->input[i]);
			p.inventoryPrediction[resIndex]--;
			if(p.inventoryPrediction[resIndex] < 0) {
				canAfford = false;
			}
		}
		// if the resources are there
		if(canAfford) {
			Objective userObj(*o);
			// remove the resources from the inventory
			p.inventory.Copy(p.inventoryPrediction);
			// add the silver or gold status to the achievement if applicable (i==0&&hasGold>0)
			if(p.marketCursor == 0 || p.marketCursor == 1) {
				bool giveGold = false, giveSilver = false;
				// mark that a gold or silver value has been achieved
				if(p.marketCursor == 0) { giveGold = goldLeft > 0; }
				if((p.marketCursor == 0 && goldLeft ==0)
				|| (p.marketCursor == 1 && goldLeft > 0)) { giveSilver = silverLeft>0; }
				if(giveGold){ goldLeft--; userObj.bonusPoints = 3; }
				if(giveSilver){ silverLeft--; userObj.bonusPoints = 1; }
			}
			// add achievement to the player's achievements
			p.achieved.Add(userObj);
			// remove the achievement from the achievements list
			achievements.Shift(p.marketCursor);
			// pull an achievement from the deck
			achievements[achievements.Length()-1] = achievement_deck.PopLast();
		}
	}
}
}

void UpdateUpgrade(Player& p, int userInput) {
	if(p.ui != p.lastState) {
		p.inventoryPrediction.Copy(p.inventory);
		p.upgradesMade = 0;
	}
	switch(userInput) {
	case 'w':
		if(p.upgradesMade < p.upgradeChoices && p.inventoryCursor < p.inventory.Length()-1 
		&& p.inventoryPrediction[p.inventoryCursor] > 0) {
			p.inventoryPrediction[p.inventoryCursor]--;
			p.inventoryPrediction[p.inventoryCursor+1]++;
			p.upgradesMade++;
		}
		break;
	case 's':
		if(p.upgradesMade > 0 && p.inventoryCursor > 0 
		&& p.inventoryPrediction[p.inventoryCursor] > 0
		&& p.inventory[p.inventoryCursor] < p.inventoryPrediction[p.inventoryCursor]) {
			p.inventoryPrediction[p.inventoryCursor]--;
			p.inventoryPrediction[p.inventoryCursor-1]++;
			p.upgradesMade--;
		}
		break;
	case 'a':
		p.inventoryCursor--;
		if(p.inventoryCursor<0) p.inventoryCursor = 0;
		break;
	case 'd':
		p.inventoryCursor++;
		if(p.inventoryCursor>=p.inventory.Length()) p.inventoryCursor = p.inventory.Length()-1;
		break;
	case '\n': case '\r':
		p.ui = UserControl::ui_hand;
		p.inventory.Copy(p.inventoryPrediction);
		p.upgradeChoices = 0;
		p.upgradesMade = 0;
		break;
	}
}

void UpdateAcquire(Player& p, int userInput, List<const PlayAction*>& market, List<List<int>*>& acquireBonus, 
	List<int>& resourcePutInto, VList<const PlayAction*>& play_deck){
	bool probablyPaidUp = p.marketCardToBuy == 0;
	if(!probablyPaidUp) {
		switch(userInput) {
			case 'w':
				if(resourcePutInto[p.marketCursor] == -1 && p.inventory[p.inventoryCursor] > 0) {
					resourcePutInto[p.marketCursor] = p.inventoryCursor;
					p.inventory[resourcePutInto[p.marketCursor]]--;
					(*acquireBonus[p.marketCursor])[resourcePutInto[p.marketCursor]]++;
				}
				break;
			case 'a':
				if(resourcePutInto[p.marketCursor] == -1) {
					int loopGuard = 0;
					do {
						p.inventoryCursor--;
						if(p.inventoryCursor < 0) {
							if(p.marketCursor > 0) {
								p.marketCursor--;
							} else {
								p.marketCursor = p.marketCardToBuy-1;
							}
							p.inventoryCursor = p.inventory.Length()-1;
						}
						if(loopGuard++ > p.inventory.Length()*p.marketCardToBuy) {break;}
					} while(p.inventory[p.inventoryCursor] == 0);
				} else {
					if(p.marketCursor > 0) p.marketCursor--;
				}
				break;
			case 's':
				if(resourcePutInto[p.marketCursor] != -1) {
					(*acquireBonus[p.marketCursor])[resourcePutInto[p.marketCursor]]--;
					p.inventory[resourcePutInto[p.marketCursor]]++;
					resourcePutInto[p.marketCursor] = -1;
				}
				break;
			case 'd':
				if(resourcePutInto[p.marketCursor] == -1) {
					int loopGuard = 0;
					do {
						p.inventoryCursor++;
						if(p.inventoryCursor >= p.inventory.Length()) {
							if(p.marketCursor < p.marketCardToBuy-1) {
								p.marketCursor++;
							} else {
								p.marketCursor = 0;
							}
							p.inventoryCursor = 0;
						}
						if(loopGuard++ > p.inventory.Length()*p.marketCardToBuy) {break;}
					} while(p.inventory[p.inventoryCursor] == 0);
				} else {
					if(p.marketCursor < p.marketCardToBuy-1) p.marketCursor++;
				}
				break;
			case '\n': case '\r':
				probablyPaidUp = true;
				break;
		}
	}
	if(p.marketCursor >= 0 && p.marketCursor < resourcePutInto.Length() && resourcePutInto[p.marketCursor] != -1) {
		p.inventoryCursor = resourcePutInto[p.marketCursor];
	}

	bool paidUp = false;
	if(probablyPaidUp) {
		// check if all of the market positions before this one are paid
		paidUp = true;
		for(int i = 0; i < resourcePutInto.Length() && i < p.marketCardToBuy; ++i) {
			if(resourcePutInto[i] == -1) { paidUp = false; break; }
		}
	}
	if(paidUp) {
		List<int>* bonus = acquireBonus[p.marketCardToBuy];
		p.hand.Insert(0, market[p.marketCardToBuy]);
		p.handPrediction.Copy(p.hand);
		market.Shift(p.marketCardToBuy);
		if(play_deck.Count() > 0) {
			market[market.Length()-1] = play_deck.PopLast();
		} else {
			market[market.Length()-1] = NULL;
		}
		if(bonus != NULL) {
			for(int i = 0; i < p.inventory.Length(); ++i) {
				p.inventory[i] += (*bonus)[i];
			}
			DELMEM(bonus);
		}
		acquireBonus.Shift(p.marketCardToBuy);
		acquireBonus[acquireBonus.Length()-1] = NULL;
		p.marketCardToBuy = -1;
		p.ui = UserControl::ui_cards;
	}
}

void UpdateMarket(Player& p, int userInput, List<const PlayAction*>& market, List<List<int>*>& acquireBonus, List<int>& resourcePutInto) {
	switch(userInput) {
	case 'a':
		p.marketCursor--; if(p.marketCursor < 0) { p.marketCursor = 0; }
		break;
	case 'd':
		p.marketCursor++; if(p.marketCursor >= market.Length()) { p.marketCursor = market.Length()-1; }
		break;
	case 'w':
		p.ui = UserControl::ui_objectives;
		break;
	case 's':
		p.ui = UserControl::ui_hand;
		break;
	case '\n': case '\r': {
		int total = p.inventory.Sum();
		if(total < p.marketCursor) {
			printf("too expensive, cannot afford.\n");
		} else {
			resourcePutInto.SetAll(-1);
			p.marketCardToBuy = p.marketCursor;
			p.marketCursor = 0;
			for(int i = 0; i < p.marketCardToBuy; ++i) {
				if(acquireBonus[i] == NULL) {
					acquireBonus[i] = NEWMEM(List<int>(p.inventory.Length(), 0));
				}
			}
			p.ui = ui_acquire;
		}
	}
}
}

void UpdateInventory(Player& p, int userInput) {
	switch(userInput) {
		case 'w':
			p.inventoryPrediction[p.inventoryCursor] = clampint(p.inventoryPrediction[p.inventoryCursor]+1,0,p.inventory[p.inventoryCursor]);
			break;
		case 's':
			p.inventoryPrediction[p.inventoryCursor] = clampint(p.inventoryPrediction[p.inventoryCursor]-1,0,p.inventory[p.inventoryCursor]);
			break;
		case 'a':
			p.inventoryCursor--;
			if(p.inventoryCursor<0) p.inventoryCursor = 0;
			break;
		case 'd':
			p.inventoryCursor++;
			if(p.inventoryCursor>=p.inventory.Length()) p.inventoryCursor = p.inventory.Length()-1;
			break;
		case '\n': case '\r':
			p.ui = UserControl::ui_hand;
			p.inventory.Copy(p.inventoryPrediction);
			break;
	}
}

void UpdateHand (Player& p, int userInput, int count) {
	switch (userInput) {
	case 'w':
		p.currentRow--;
		if (p.currentRow <= p.handOffset) {
			p.handOffset += p.currentRow - p.handOffset;
		}
		if (p.handOffset < 0) {
			p.handOffset = 0;
		}
		if (p.currentRow < 0) {
			p.currentRow = 0;
			p.ui = UserControl::ui_cards;
		}
		break;
	case 's':
		p.currentRow++;
		if (p.currentRow >= p.handOffset + count - 1) {
			p.handOffset += p.currentRow - (p.handOffset + count) + 1;
		}
		if(p.handOffset+count > p.playedPrediction.Count () + p.playedPrediction.Count ()) {
			p.handOffset = p.handPrediction.Count () + p.playedPrediction.Count () - count;
		}
		if (p.currentRow >= p.handPrediction.Count () + p.playedPrediction.Count ()) {
			p.currentRow = p.handPrediction.Count () + p.playedPrediction.Count () - 1;
			//ui = UserControl::ui_inventory;
		}
		break;
	case 'd': {
		int* isSelected = p.selectedMark.GetPtr (p.currentRow);
		if (isSelected == NULL) {
			if(p.currentRow < p.handPrediction.Count()){
				p.selected.Add (p.currentRow);
				p.selectedMark.Set (p.currentRow, 1);
				RefreshPrediction(p.upgradeChoices, p.validPrediction, p.selected, p.inventory, p.inventoryPrediction, p.handPrediction, p.playedPrediction);
				if(p.upgradeChoices != 0) { p.ui = UserControl::ui_upgrade; } // TODO add code to make the inventory numbers show up, and the upgrade message too.
			}
		}
	} break;
	case 'a': {
		int* isSelected = p.selectedMark.GetPtr (p.currentRow);
		if (isSelected != NULL) {
			int sindex = p.selected.IndexOf (p.currentRow);
			p.selected.RemoveAt (sindex);
			const PlayAction* card = p.hand[p.currentRow];
			p.selectedMark.Remove (p.currentRow);
			if(card->input == "reset" || p.selected.Count() == 0){
				p.handPrediction.Copy(p.hand);
				p.playedPrediction.Copy(p.played);
				// TODO remove cards from unplayed (index > hand.Count()) from selected and selectedMark
			}
			RefreshPrediction(p.upgradeChoices, p.validPrediction, p.selected, p.inventory, p.inventoryPrediction, p.handPrediction, p.playedPrediction);
		}
	} break;
	case '\n':
	case '\r':
		if (p.selected.Count () > 0) {
			if (p.currentRow >= 0 && p.currentRow <= p.hand.Count ()) {
				// remove the selected from the hand into a moving list, ordered by selected
				VList<const PlayAction*> reordered;
				for (int i = 0; i < p.selected.Count (); ++i) {
					int index = p.selected[i];
					if(index < p.hand.Count()) {
						reordered.Add (p.hand[index]);
					} // don't insert played cards
				}
				p.selected.Sort ();
				for (int i = p.selected.Count () - 1; i >= 0; --i) {
					int index = p.selected[i];
					if(index < p.hand.Count()) {
						p.hand.RemoveAt (index);
						if (p.selected[i] < p.currentRow)
							p.currentRow--;
					 } // don't insert played cards
				}
				// clear selected
				p.selected.Clear ();
				p.selectedMark.Clear ();
				p.validPrediction = PredictionState::none;
				// insert element into the list at the given index
				p.hand.Insert (p.currentRow, reordered.GetData (), reordered.Count ());
				reordered.Clear ();
				p.inventoryPrediction.Copy(p.inventory);
				p.handPrediction.Copy(p.hand);
				p.playedPrediction.Copy(p.played);
			}
		} else {
			printf("row %d   \n", p.currentRow);
			if (p.currentRow >= 0 && p.currentRow < p.hand.Count ()) {
				const PlayAction* toPlay = p.hand[p.currentRow];
				printf("   %s   \n",toPlay->input.c_str());
				bool canDoIt = CanPlay (toPlay, p.inventory);
				if (canDoIt) {
					SubtractResources (toPlay, p.inventory);
					AddResources (p.upgradeChoices, toPlay, p.inventory, p.hand, p.played);
					if(p.upgradeChoices != 0) { p.ui = UserControl::ui_upgrade; }
					p.hand.RemoveAt (p.currentRow);
					if (toPlay->output == "cards") {
						p.hand.Add (toPlay);
					} else {
						p.played.Add (toPlay);
					}
					p.inventoryPrediction.Copy(p.inventory);
					p.handPrediction.Copy(p.hand);
					p.playedPrediction.Copy(p.played);
				}
			}
		}
	}
}

void PrintHand (Coord pos, int count, Player& p) {
	CLI::setColor(-1,-1);
	int limit = p.hand.Count () + p.played.Count ();
	int extraSpaces = count - (limit - p.handOffset);
	if (extraSpaces < 0) {
		limit += extraSpaces;
	}
	int bg = CLI::COLOR::DARK_GRAY;
	for (int i = p.handOffset; i < limit; ++i) {
		CLI::move (pos.y + i - p.handOffset, pos.x);
		bool isplayed = i >= p.hand.Count ();
		if (p.ui == UserControl::ui_hand && i == p.currentRow) {
			print ((!isplayed ? ">" : "x"));
		} else {
			print (" ");
		}
		bool isSelected = !isplayed && p.selectedMark.GetPtr (i) != NULL;
		// indent selected cards slightly
		if (isSelected) {
			print (">");
		}
		const PlayAction* card = NULL;
		int bg = CLI::COLOR::BLACK;
		if (i < p.hand.Count ()) {
			card = p.hand[i];
			bg = CLI::COLOR::DARK_GRAY;
		} else if (i < p.hand.Count () + p.played.Count ()) {
			card = p.played[i - p.hand.Count ()];
			bg = CLI::COLOR::BLACK;
		}
		PrintAction (card, bg);
		if (isSelected) {
			int selectedIndex = p.selected.IndexOf (i);
			CLI::printf ("%2d", selectedIndex);
		} else {
			CLI::putchar((p.ui == UserControl::ui_hand && i == p.currentRow)?(!isplayed?'<':'x'):' ');
			if (CanPlay (card, p.inventory)) {
				print (" .");
			} else {
				print ("  ");
			}
		}
	}
	CLI::setColor (CLI::COLOR::GREEN);
	for (int i = 0; i < extraSpaces; ++i) {
		CLI::move (pos.y + limit + i - p.handOffset, pos.x);
		CLI::printf ("..............");
	}
}

void PrintInventory(int background, int numberWidth, bool showZeros, List<int> & inventory, const VList<const ResourceType*>& collectableResources, Coord pos, int selected) {
	CLI::move (pos.y, pos.x);
	CLI::setColor (CLI::COLOR::LIGHT_GRAY, background);
	char formatBuffer[10];
	sprintf(formatBuffer, "%%%dd", numberWidth);
	for (int i = 0; i < inventory.Length (); ++i) {
		if(i == selected) {
			CLI::setColor (CLI::COLOR::WHITE, background); CLI::putchar('>');
		} else { CLI::putchar(' '); }
		CLI::setColor (collectableResources[i]->color);
		if(showZeros || inventory[i] != 0) {
			CLI::printf (formatBuffer, inventory[i]);
			if(i == selected) {
				CLI::setColor (CLI::COLOR::WHITE, background); CLI::putchar('<');
			} else { CLI::printf ("%c", collectableResources[i]->icon); }
		} else {
			for(int c=0;c<numberWidth;++c) { CLI::putchar(' '); }
			if(i == selected) {
				CLI::setColor (CLI::COLOR::WHITE, background); CLI::putchar('<');
			} else { CLI::putchar(' '); }
		}
	}
}

void PrintResourcesInventory(Coord cursor, Player& p,
	VList<const ResourceType*>& collectableResources, int maxInventory){
	const int numberWidth = 2;
	CLI::setColor (-1, -1);
	if(p.validPrediction == PredictionState::valid || p.validPrediction == PredictionState::invalid) {
		PrintInventory(CLI::COLOR::DARK_GRAY, numberWidth, true, p.inventory, collectableResources, cursor, 
			(p.ui==UserControl::ui_inventory)?p.inventoryCursor:-1); cursor.y+=1;
		// draw fake resources... or warning message
		PrintInventory(p.validPrediction?CLI::COLOR::BLACK:CLI::COLOR::RED, numberWidth, true, p.inventoryPrediction, collectableResources, cursor, 
			(p.ui==UserControl::ui_inventory)?p.inventoryCursor:-1);
	} else {
		if(p.ui==UserControl::ui_inventory || p.ui==UserControl::ui_upgrade) {
			// if modifying the inventory, show the prediction, which is the modifying list
			PrintInventory(CLI::COLOR::DARK_GRAY, numberWidth, true, p.inventoryPrediction, collectableResources, cursor, p.inventoryCursor);
		} else {
			PrintInventory(CLI::COLOR::DARK_GRAY, numberWidth, true, p.inventory, collectableResources, cursor, -1);
		}
		cursor.y+=1;
		CLI::move(cursor);
		if(p.upgradeChoices == 0) {
			int total = p.inventoryPrediction.Sum();
			CLI::setColor(CLI::COLOR::WHITE, (total<=maxInventory)?CLI::COLOR::BLACK:CLI::COLOR::RED);
			printf("resources:%3d/%2d", total, maxInventory);
		} else {
			CLI::setColor(CLI::COLOR::WHITE, (p.upgradesMade<p.upgradeChoices)?CLI::COLOR::RED:CLI::COLOR::BLACK);
			printf("+ upgrades:%2d/%2d", p.upgradesMade, p.upgradeChoices);
		}
	}

}

void PrintAchievements(Coord cursor, const List<const Objective*>& achievements, List<const Player*>& players){
	for(int i = 0; i < achievements.Length(); ++i) {
		CLI::move(cursor);
		CLI::setColor(CLI::COLOR::WHITE, -1);
		const Player* p = NULL;
		for(int pi = 0; pi < players.Length(); ++pi) {
			if(players[pi]->ui == UserControl::ui_objectives && players[pi]->marketCursor == i){
				p = players[pi];
				break;
			}
		}
		putchar((p)?'>':' ');
		const Objective* o = achievements[i];
		PrintObjective(o, CLI::COLOR::BLACK);
		CLI::setColor(CLI::COLOR::WHITE, -1);
		putchar((p)?'<':' ');
		cursor.y++;
		CLI::move(cursor);
		CLI::setColor(CLI::COLOR::LIGHT_GRAY, -1);
		if(i > 1) {CLI::putchar(' ');}
		CLI::printf("   %d", o->points);
		if(i == 0) {
			CLI::setColor(CLI::COLOR::BRIGHT_YELLOW, -1); CLI::printf(" +3");
		} else if(i == 1) {
			CLI::setColor(CLI::COLOR::LIGHT_GRAY, -1); CLI::printf(" +1");
		}
		cursor.y--;
		cursor.x += 14;
	}
}

void PrintMarket(Coord cursor, List<const PlayAction*>& market, List<const Player*>& players, 
	List<List<int>*>& acquireBonus, List<int>& resourcePutInto, const VList<const ResourceType*>& collectableResources) {
	const Player* currentPlayer = players[0];
	int totalResources = currentPlayer->inventory.Sum();
	for(int i = 0; i < market.Length(); ++i) {
		CLI::move(cursor);
		CLI::setColor(CLI::COLOR::WHITE, -1);
		const Player* p = NULL;
		for(int pi = 0; pi < players.Length(); ++pi) {
			if(players[pi]->marketCursor == i 
			&&(players[pi]->ui == UserControl::ui_cards || players[pi]->ui == UserControl::ui_acquire)) {
				p = players[pi];
				break;
			}
		}
		putchar((p)?'>':' ');
		PrintAction(market[i], (totalResources >= i)?CLI::COLOR::DARK_GRAY:CLI::COLOR::BLACK);
		CLI::setColor(CLI::COLOR::WHITE, -1);
		putchar((p)?'<':' ');
		cursor.y++;
		CLI::move(cursor);
		if(acquireBonus[i] != NULL) {
			CLI::setColor(CLI::COLOR::WHITE, -1);
			int background = -1;
			if(currentPlayer && i < currentPlayer->marketCardToBuy && resourcePutInto[i] == -1) {
				background = CLI::COLOR::RED;
			}
			PrintInventory(background, 1, (p != NULL && currentPlayer->ui == UserControl::ui_acquire), *(acquireBonus[i]), collectableResources, cursor, 
				(p && i < currentPlayer->marketCardToBuy)?currentPlayer->inventoryCursor:-1);
			CLI::setColor(CLI::COLOR::WHITE, -1);
		} else {
			CLI::setColor(-1, -1);
			// if(marketCursor == i) {
			// 	CLI::printf(" -- -- -- -- ");
			// } else {
				CLI::printf("             ");
			// }
		}
		cursor.y--;
		cursor.x += 14;
	}
}

void PrintUserState(Coord cursor, const Player & p) {
	CLI::move (cursor); CLI::setColor (CLI::COLOR::WHITE, -1); CLI::printf ("%s", p.name.c_str());
	cursor.x += 15;
	for(int i = 0; i < p.achieved.Count(); ++i) {
		CLI::move (cursor);
		int pts = p.achieved[i].bonusPoints;
		char f = (pts == 3)?CLI::COLOR::BRIGHT_YELLOW:(pts == 1)?CLI::COLOR::WHITE:CLI::COLOR::DARK_GRAY;
		char b = (pts == 3)?CLI::COLOR::YELLOW:       (pts == 1)?CLI::COLOR::DARK_GRAY:CLI::COLOR::BLACK;
		CLI::setColor(f, b);
		CLI::putchar('*');
		cursor.x-=1;
	}

}


int main (int argc, const char** argv) {
	for (int i = 0; i < argc; ++i) { printf ("[%d] %s\n", i, argv[i]); }

	VList<const ResourceType*> collectableResources;
	for (int i = 0; i < g_len_resources; ++i) {
		resourceLookup.Set (g_resources[i].icon, &(g_resources[i]));
		if (g_resources[i].type == ResourceType::Type::resource) {
			collectableResources.Add (&(g_resources[i]));
			resourceIndex.Set (g_resources[i].icon, i);
		}
	}

	Dictionary<std::string, const PlayAction*> actionDeck;
	VList<const PlayAction*> play_deck;
	play_deck.EnsureCapacity(g_len_play_deck);
	for (int i = 0; i < g_len_play_deck; ++i) {
		actionDeck.Set (g_play_deck[i].input, &(g_play_deck[i]));
		play_deck.Add (&(g_play_deck[i]));
	}
	VList<const Objective*> achievement_deck;
	achievement_deck.EnsureCapacity(g_len_objective);
	for (int i = 0; i < g_len_objective; ++i) {
		achievement_deck.Add (&(g_objective[i]));
	}

	printf ("%d possible actions\n", actionDeck.Count ());

	platform_shuffle (play_deck.GetData (), 0, play_deck.Count ());
	Player p;
	p.Set("Mr. V", collectableResources.Count ());
	const int marketCards = 6;
	const int achievementCards = 5;
	List<const PlayAction*> market(marketCards);
	List<const Objective*> achievements(achievementCards);
	List<List<int>*> acquireBonus(marketCards, NULL);
	List<int> resourcePutInto(market.Length()-1, 0); // used during acquire and upgrade UI
	
	for (int i = 0; i < achievementCards; ++i) {
		achievements.Set(i, achievement_deck.PopLast ());
	}
	for (int i = 0; i < marketCards; ++i) {
		market.Set(i, play_deck.PopLast ());
	}
	p.Add(g_playstart, g_len_playstart);
	p.Draw(play_deck, 3);
	for (int i = 0; i < 7; ++i) {
		p.played.Add (play_deck.PopLast ());
	}
	p.handPrediction.Copy(p.hand);
	p.playedPrediction.Copy(p.played);

	p.inventory[0] = 2; // start with 2 basic resource

	CLI::init ();
	CLI::setSize (CLI::getWidth (), 25);
	CLI::fillScreen (' ');
	CLI::move (0, 0);
	CLI::printf ("Hello World!");

	bool running = true;
	int count = 10;
	Coord cursor;
	int goldLeft = 5, silverLeft = 5;
	int height = CLI::getHeight();
	int maxInventory = 10;
	List<const Player*> playerUIOrder(1); // TODO change the order as the players turn changes. order should be who-is-going-next, with the current-player at the top.
	playerUIOrder[0] = &p;
	while (running) {
		PrintHand (Coord(1,9), count, p);
		PrintAchievements(Coord(1,2), achievements, playerUIOrder);
		PrintMarket(Coord(1,6), market, playerUIOrder, acquireBonus, resourcePutInto, collectableResources);
		PrintResourcesInventory(Coord(1,20), p, collectableResources, maxInventory);
		PrintUserState(Coord(1,23), p);

		// input
		CLI::setColor(CLI::COLOR::LIGHT_GRAY, -1); // reset color
		CLI::move(0,0);
		int userInput = CLI::getch ();
		// update
		switch (userInput) {
		case 27:
			printf("quitting...                ");
			running = false;
			break;
		default:
			switch(p.ui) {
			case UserControl::ui_hand:
				printf("hand management        ");
				UpdateHand (p, userInput, count);
				// on hand exit, clear p.selected, reset predictions to current state
				break;
			case UserControl::ui_inventory:
				printf("resource management        ");
				// on enter, prediction should be a copy of inventory. modify prediction
				UpdateInventory(p, userInput);
				break;
			case UserControl::ui_cards:
				printf("select cards              ");
				UpdateMarket(p, userInput, market, acquireBonus, resourcePutInto);
				break;
			case UserControl::ui_acquire:
				printf("acquire card              ");
				UpdateAcquire(p, userInput, market, acquireBonus, resourcePutInto, play_deck);
				break;
			case UserControl::ui_upgrade:
				UpdateUpgrade(p, userInput);
				break;
			case UserControl::ui_objectives:
				UpdateObjectiveBuy(p, userInput, goldLeft, silverLeft, achievements, achievement_deck);
				break;
			}
			break;
		}
		p.lastState = p.ui;
	}
	for(int i = 0; i < acquireBonus.Length(); ++i) {
		if(acquireBonus[i] != NULL) {
			DELMEM(acquireBonus[i]);
			acquireBonus[i] = NULL;
		}
	}
	CLI::release ();

	return 0;
}