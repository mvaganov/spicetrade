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


void UpdateObjectiveBuy(Game&g, Player& p, int userInput){
	switch(userInput) {
	case Game::MOVE_LEFT:
		p.marketCursor--; if(p.marketCursor < 0) { p.marketCursor = 0; }
		break;
	case Game::MOVE_RIGHT:
		p.marketCursor++; if(p.marketCursor >= g.achievements.Length()) { p.marketCursor = g.achievements.Length()-1; }
		break;
	case Game::MOVE_DOWN:
		p.ui = UserControl::ui_cards;
		break;
	case Game::MOVE_ENTER: case Game::MOVE_ENTER2: {
		const Objective * o = g.achievements[p.marketCursor];
		// determine costs (if the objective is there)
		bool canAfford = o != NULL;
		if(canAfford) {
			p.inventoryPrediction.Copy(p.inventory);
			// check if the inventory has sufficient resources
			for(int i = 0; i < o->input.length(); ++i) {
				int resIndex = g.resourceIndex.Get(o->input[i]);
				p.inventoryPrediction[resIndex]--;
				if(p.inventoryPrediction[resIndex] < 0) {
					canAfford = false;
				}
			}
		}
		// if the resources are there to pay the cost
		if(canAfford) {
			Objective userObj(*o);
			// remove the resources from the inventory
			p.inventory.Copy(p.inventoryPrediction);
			// add the silver or gold status to the achievement if applicable (i==0&&hasGold>0)
			if(p.marketCursor == 0 || p.marketCursor == 1) {
				bool giveGold = false, giveSilver = false;
				// mark that a gold or silver value has been achieved
				if(p.marketCursor == 0) { giveGold = g.goldLeft > 0; }
				if((p.marketCursor == 0 && g.goldLeft ==0)
				|| (p.marketCursor == 1 && g.goldLeft > 0)) { giveSilver = g.silverLeft>0; }
				if(giveGold){ g.goldLeft--; userObj.bonusPoints = 3; }
				if(giveSilver){ g.silverLeft--; userObj.bonusPoints = 1; }
			}
			// add achievement to the player's achievements
			p.achieved.Add(userObj);
			// remove the achievement from the achievements list
			g.achievements.Shift(p.marketCursor);
			// pull an achievement from the deck
			g.achievements[g.achievements.Length()-1] = (g.achievement_deck.Count() > 0)
				?g.achievement_deck.PopLast() : NULL;
		}
	}
	}
}

void UpdateAcquireMarket(Player& p, Game& g, int userInput){
	bool probablyPaidUp = p.marketCardToBuy == 0;
	if(!probablyPaidUp) {
		switch(userInput) {
			case Game::MOVE_UP:
				if(g.resourcePutInto[p.marketCursor] == -1 && p.inventory[p.inventoryCursor] > 0) {
					g.resourcePutInto[p.marketCursor] = p.inventoryCursor;
					p.inventory[g.resourcePutInto[p.marketCursor]]--;
					(*g.acquireBonus[p.marketCursor])[g.resourcePutInto[p.marketCursor]]++;
				}
				break;
			case Game::MOVE_LEFT:
				if(g.resourcePutInto[p.marketCursor] == -1) {
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
			case Game::MOVE_DOWN:
				if(g.resourcePutInto[p.marketCursor] != -1) {
					(*g.acquireBonus[p.marketCursor])[g.resourcePutInto[p.marketCursor]]--;
					p.inventory[g.resourcePutInto[p.marketCursor]]++;
					g.resourcePutInto[p.marketCursor] = -1;
				}
				break;
			case Game::MOVE_RIGHT:
				if(g.resourcePutInto[p.marketCursor] == -1) {
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
			case Game::MOVE_ENTER: case Game::MOVE_ENTER2:
				probablyPaidUp = true;
				break;
		}
	}
	if(p.marketCursor >= 0 && p.marketCursor < g.resourcePutInto.Length() && g.resourcePutInto[p.marketCursor] != -1) {
		p.inventoryCursor = g.resourcePutInto[p.marketCursor];
	}

	bool paidUp = false;
	if(probablyPaidUp) {
		// check if all of the market positions before this one are paid
		paidUp = true;
		for(int i = 0; i < g.resourcePutInto.Length() && i < p.marketCardToBuy; ++i) {
			if(g.resourcePutInto[i] == -1) { paidUp = false; break; }
		}
	}
	if(paidUp) {
		List<int>* bonus = g.acquireBonus[p.marketCardToBuy];
		p.hand.Insert(0, g.market[p.marketCardToBuy]);
		p.handPrediction.Copy(p.hand);
		g.market.Shift(p.marketCardToBuy);
		g.market[g.market.Length()-1] = (g.play_deck.Count() > 0)?g.play_deck.PopLast():NULL;
		if(bonus != NULL) {
			for(int i = 0; i < p.inventory.Length(); ++i) {
				p.inventory[i] += (*bonus)[i];
			}
			DELMEM(bonus);
		}
		g.acquireBonus.Shift(p.marketCardToBuy);
		g.acquireBonus[g.acquireBonus.Length()-1] = NULL;
		p.marketCardToBuy = -1;
		p.ui = UserControl::ui_cards;
	}
}

void UpdateMarket(Player& p, int userInput, List<const PlayAction*>& market, List<List<int>*>& acquireBonus, List<int>& resourcePutInto) {
	switch(userInput) {
	case Game::MOVE_LEFT:
		p.marketCursor--; if(p.marketCursor < 0) { p.marketCursor = 0; }
		break;
	case Game::MOVE_RIGHT:
		p.marketCursor++; if(p.marketCursor >= market.Length()) { p.marketCursor = market.Length()-1; }
		break;
	case Game::MOVE_UP:
		p.ui = UserControl::ui_objectives;
		break;
	case Game::MOVE_DOWN:
		p.ui = UserControl::ui_hand;
		break;
	case Game::MOVE_ENTER: case Game::MOVE_ENTER2: {
		int total = p.inventory.Sum();
		if(total < p.marketCursor || market[p.marketCursor] == NULL) {
			printf("too expensive, cannot afford.\n");
		} else {
			printf("0x%08x", market[p.marketCursor]);
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
		case Game::MOVE_UP:
			p.inventoryPrediction[p.inventoryCursor] = clampint(p.inventoryPrediction[p.inventoryCursor]+1,0,p.inventory[p.inventoryCursor]);
			break;
		case Game::MOVE_DOWN:
			p.inventoryPrediction[p.inventoryCursor] = clampint(p.inventoryPrediction[p.inventoryCursor]-1,0,p.inventory[p.inventoryCursor]);
			break;
		case Game::MOVE_LEFT:
			p.inventoryCursor--;
			if(p.inventoryCursor<0) p.inventoryCursor = 0;
			break;
		case Game::MOVE_RIGHT:
			p.inventoryCursor++;
			if(p.inventoryCursor>=p.inventory.Length()) p.inventoryCursor = p.inventory.Length()-1;
			break;
		case Game::MOVE_ENTER: case Game::MOVE_ENTER2:
			p.ui = UserControl::ui_hand;
			p.inventory.Copy(p.inventoryPrediction);
			break;
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

void PrintAchievements(Game& g, Coord cursor){
	for(int i = 0; i < g.achievements.Length(); ++i) {
		CLI::move(cursor);
		CLI::setColor(CLI::COLOR::WHITE, -1);
		const Player* p = NULL;
		for(int pi = 0; pi < g.playerUIOrder.Length(); ++pi) {
			Player * iter = g.playerUIOrder[pi];
			if(iter->ui == UserControl::ui_objectives && iter->marketCursor == i){
				p = iter;
				break;
			}
		}
		putchar((p)?'>':' ');
		const Objective* o = g.achievements[i];
		Objective::PrintObjective(g, o, CLI::COLOR::BLACK);
		CLI::setColor(CLI::COLOR::WHITE, -1);
		putchar((p)?'<':' ');
		cursor.y++;
		CLI::move(cursor);
		CLI::setColor(CLI::COLOR::LIGHT_GRAY, -1);
		if(i > 1) {CLI::putchar(' ');}
		CLI::printf("   %d", o?o->points:0);
		if(i == 0 && g.goldLeft > 0) {
			CLI::setColor(CLI::COLOR::BRIGHT_YELLOW, -1); CLI::printf(" +3  ");
		} else if(
			((i == 1 && g.goldLeft > 0) 
		|| (i == 0 && g.goldLeft == 0)) && g.silverLeft > 0) {
			CLI::setColor(CLI::COLOR::LIGHT_GRAY, -1); CLI::printf(" +1  ");
		} else {
			CLI::setColor(-1, -1); CLI::printf("     ");
		}
		cursor.y--;
		cursor.x += 14;
	}
}

void PrintMarket(Game& g, Coord cursor, List<const PlayAction*>& market, const List<Player*>& players, 
	List<List<int>*>& acquireBonus, List<int>& resourcePutInto) {
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
		PlayAction::PrintAction(g, market[i], (totalResources >= i)?CLI::COLOR::DARK_GRAY:CLI::COLOR::BLACK);
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
			PrintInventory(background, 1, (p != NULL && currentPlayer->ui == UserControl::ui_acquire), *(acquireBonus[i]), 
				g.collectableResources, cursor, 
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
	CLI::move (cursor);
	CLI::setColor (CLI::COLOR::WHITE, -1); CLI::printf ("%s", p.name.c_str());
	cursor.y ++; CLI::move (cursor); CLI::printf(p.uimode.c_str()); cursor.y --;
	cursor.x += 15;
	for(int i = 0; i < p.achieved.Count(); ++i) {
		CLI::move (cursor);
		int pts = p.achieved[i].bonusPoints;
		char f = (pts == 3)?CLI::COLOR::BRIGHT_YELLOW:(pts == 1)?CLI::COLOR::WHITE    :CLI::COLOR::LIGHT_GRAY;
		char b = (pts == 3)?CLI::COLOR::YELLOW:       (pts == 1)?CLI::COLOR::DARK_GRAY:CLI::COLOR::BLACK     ;
		CLI::setColor(f, b);
		CLI::putchar('*');
		cursor.x-=1;
	}
}

void UpdateInput(Player& p, Game& g, int move) {
	switch(p.ui) {
	case UserControl::ui_hand:
		p.uimode = "  card management  ";
		Player::UpdateHand (g, p, move, g.handDisplayCount);
		// on hand exit, clear p.selected, reset predictions to current state
		break;
	case UserControl::ui_inventory:
		p.uimode = "resource management";
		// on enter, prediction should be a copy of inventory. modify prediction
		UpdateInventory(p, move);
		break;
	case UserControl::ui_cards:
		p.uimode = "selecting next card";
		printf("select cards              ");
		UpdateMarket(p, move, g.market, g.acquireBonus, g.resourcePutInto);
		break;
	case UserControl::ui_acquire:
		p.uimode = "acquiring next card";
		printf("acquire card              ");
		UpdateAcquireMarket(p, g, move);
		break;
	case UserControl::ui_upgrade:
		p.uimode = "upgrading resources";
		Player::UpdateUpgrade(p, move);
		break;
	case UserControl::ui_objectives:
		p.uimode = "selecting objective";
		UpdateObjectiveBuy(g, p, move);
		break;
	}
	p.lastState = p.ui;
}

int main (int argc, const char** argv) {
	for (int i = 0; i < argc; ++i) { printf ("[%d] %s\n", i, argv[i]); }

	Game g(3, CLI::getWidth (), 25);
	
	while (g.running) {
		for(int i = 0; i < g.GetPlayerCount(); ++i) {
			int column = 1+(i*20);
			Player& p = *g.GetPlayer(i);
			Player::PrintHand (g, Coord(column,9), g.handDisplayCount, p);
			PrintResourcesInventory(Coord(column,20), p, g.collectableResources, g.maxInventory);
			PrintUserState(Coord(column,23), p);
		}
		PrintAchievements(g, Coord(1,2));
		PrintMarket(g, Coord(1,6), g.market, g.playerUIOrder, g.acquireBonus, g.resourcePutInto);

		// input
		CLI::setColor(CLI::COLOR::LIGHT_GRAY, -1); // reset color
		CLI::move(0,0);
		g.setInput(CLI::getch ());
		// update
		switch (g.userInput) {
		case 27:
			printf("quitting...                ");
			g.running = false;
			break;
		default: {
			int playerID, moveID;
			g.ConvertKeyToPlayerMove(g.userInput, playerID, moveID);
			printf("%d (%c) -> %d,%d\n", g.userInput,g.userInput,playerID, moveID);
			if(playerID >= 0) {
				Player& p = *g.GetPlayer(playerID);
				UpdateInput(p, g, moveID);
			}
		}	break;
		}
	}
	for(int i = 0; i < g.acquireBonus.Length(); ++i) {
		if(g.acquireBonus[i] != NULL) {
			DELMEM(g.acquireBonus[i]);
			g.acquireBonus[i] = NULL;
		}
	}
	CLI::release ();

	return 0;
}