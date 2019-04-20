#include "game.h"
#include "player.h"
#include "playerstate.h"

void Player::Init(Game& g) {
}

void Player::SetConsoleColor(Game& g) const {
	if(g.GetCurrentPlayer() != this) {
		CLI::setColor(fcolor,bcolor);
	} else {
		SetConsoleColorBlink();
	}
}

void Player::SetConsoleColorBlink() const {
	if((CLI::upTimeMS() & (1 << 9)) == 0){
		CLI::setColor(bcolor,fcolor);
	} else {
		CLI::setColor(fcolor,bcolor);
	}
}

bool Player::CanAfford (Game& g, const std::string& cost, const List<int>& inventory) {
	List<int> testInventory (inventory); // TODO turn this into a static array and memcpy inventory?
	SubtractResources (g, cost, testInventory);
	return InventoryValid(testInventory);
}

bool Player::SubtractResources (Game& g, const std::string& cost, List<int>& inventory) {
	if (cost == "reset") {
		return true;
	}
	bool valid = true;
	for (int i = 0; i < cost.length (); ++i) {
		char c = cost[i];
		if (c == '.') {

		} else {
			int* index = g.resourceIndex.GetPtr (c);
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

bool Player::InventoryValid(const List<int>& inventory) {
	bool valid = true;
	for (int i = 0; i < inventory.Length (); ++i) {
		if (inventory[i] < 0) { valid = false; }
	}
	return valid;
}

void Player::AddResources (Game& g, int& upgradesToDo, const std::string& resources, List<int>& inventory, VList<const PlayAction*>& hand, VList<const PlayAction*>& played) {
	if (resources == "cards") {
		hand.Insert (hand.Count (), played.GetData (), played.Count ());
		played.Clear ();
	} else {
		for (int i = 0; i < resources.length (); ++i) {
			char c = resources[i];
			if (c == '+') {
				upgradesToDo++;
			} else {
				int* index = g.resourceIndex.GetPtr (c);
				if (index == NULL) {
					printf ("no such resource %c\n", c);
				}
				inventory[*index]++;
			}
		}
	}
}

bool Player::Calculate(Game& g, int& upgradesToDo, VList<const PlayAction*>& hand, VList<const PlayAction*>& played,
	VList<int>& selected, List<int>& prediction) {
	for(int i = 0; i < selected.Count(); ++i) {
		const PlayAction* card = hand[selected[i]];
		SubtractResources(g, card->input, prediction);
		AddResources(g, upgradesToDo, card->output, prediction, hand, played);
		if(!InventoryValid(prediction)) { return false; }
	}
	return true;
}

void Player::RefreshPrediction(Game& g, Player& p) {
	p.inventoryPrediction.Copy(p.inventory);
	if(p.selected.Count() == 0){
		p.validPrediction = PredictionState::none;
	} else {
		bool affordable = Player::Calculate(g, p.upgradeChoices, p.handPrediction, p.playedPrediction, p.selected, p.inventoryPrediction);
		p.validPrediction = affordable?PredictionState::valid:PredictionState::invalid;
	}
}

bool MorphResource(int index, List<int>& inventory, bool upgrade) {
	int whereToTake = index, whereToGive = index + (upgrade?+1:-1);
	int len = inventory.Length();
	if(whereToGive >= len){whereToGive -= len;}
	if(whereToGive < 0)   {whereToGive += len;}
	if(inventory[whereToTake] > 0) {
		inventory[whereToTake]--;
		inventory[whereToGive]++;
		return true;
	}
	return false;
}

void MoveInventoryCursor(Player& p, bool right) {
	if(right) {
		p.inventoryCursor++;
		if(p.inventoryCursor>=p.inventory.Length()) p.inventoryCursor = p.inventory.Length()-1;
	} else {
		p.inventoryCursor--;
		if(p.inventoryCursor<0) p.inventoryCursor = 0;
	}
}

void Player::UpdateUpgrade(Game& g, Player& p, int userInput) {
	switch(userInput) {
	case Game::MOVE_UP:
		if(p.upgradesMade < p.upgradeChoices
		&& MorphResource(p.inventoryCursor, p.inventoryPrediction, true)) {
			p.upgradesMade++;
		}
		break;
	case Game::MOVE_DOWN:
		if(p.upgradesMade > 0
		&& p.inventory[p.inventoryCursor] < p.inventoryPrediction[p.inventoryCursor]
		&& MorphResource(p.inventoryCursor, p.inventoryPrediction, false)) {
			p.upgradesMade--;
		}
		break;
	case Game::MOVE_LEFT: MoveInventoryCursor(p, false); break;
	case Game::MOVE_RIGHT: MoveInventoryCursor(p, true); break;
	case Game::MOVE_ENTER: case Game::MOVE_ENTER2:
		p.SetUIState<HandManage>(g,p);
		break;
	}
}

void Player::UpdateHand (Game& g, Player& p, int userInput, int count) {
	switch (userInput) {
	case Game::MOVE_UP:
		p.currentRow--;
		if (p.currentRow <= p.handOffset) {
			p.handOffset += p.currentRow - p.handOffset;
		}
		if (p.handOffset < 0) {
			p.handOffset = 0;
		}
		if (p.currentRow < 0) {
			p.currentRow = 0;
			p.SetUIState<CardBuy>(g,p);
		}
		break;
	case Game::MOVE_DOWN: {
		p.currentRow++;
		if (p.currentRow >= p.handOffset + count - 1) {
			p.handOffset += p.currentRow - (p.handOffset + count) + 1;
		}
		int total = p.handPrediction.Count () + p.playedPrediction.Count ();
		if (p.handOffset > total - count) {
			p.handOffset = total - count;
		}
		if (p.currentRow >= total) {
			p.currentRow = total - 1;
		}
		if(p.handOffset < 0) { p.handOffset = 0; }
	}	break;
	case Game::MOVE_RIGHT: {
		printf("prediction algorithms disabled\n"); break;
		int* isSelected = p.selectedMark.GetPtr (p.currentRow);
		if (isSelected == NULL) {
			if(p.currentRow < p.handPrediction.Count()){
				p.selected.Add (p.currentRow);
				p.selectedMark.Set (p.currentRow, 1);
				RefreshPrediction(g, p);
				if(p.upgradeChoices != 0) {
					p.SetUIState<ResUpgrade>(g,p);
				} // TODO add code to make the inventory numbers show up, and the upgrade message too.
			}
		}
	} break;
	case Game::MOVE_LEFT: {
		printf("prediction and order adjustment algorithms disabled\n"); break;
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
			RefreshPrediction(g, p);
		}
	} break;
	case Game::MOVE_ENTER: case Game::MOVE_ENTER2:
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
		} else if(g.GetCurrentPlayer() == &p) {
			if (p.currentRow >= 0 && p.currentRow < p.hand.Count ()) {
				const PlayAction* toPlay = p.hand[p.currentRow];
				bool canDoIt = toPlay != NULL && CanAfford (g, toPlay->input, p.inventory);
				if (canDoIt) {
					PlayAction::DoIt(g, p, toPlay);
					if(p.upgradeChoices != 0) {
						p.SetUIState<ResUpgrade>(g,p);
					}
					Player::FinishTurn(g,p);
				}
			}
		}
	}
}


int clampint(int n, int mn, int mx) { return ((n<mn)?mn:((n>mx)?mx:n)); }
void Player::UpdateInventory(Game& g, Player& p, int userInput) {
	switch(userInput) {
		case Game::MOVE_UP:
			p.inventoryPrediction[p.inventoryCursor] = clampint(p.inventoryPrediction[p.inventoryCursor]+1,0,p.inventory[p.inventoryCursor]);
			break;
		case Game::MOVE_DOWN:
			p.inventoryPrediction[p.inventoryCursor] = clampint(p.inventoryPrediction[p.inventoryCursor]-1,0,p.inventory[p.inventoryCursor]);
			break;
		case Game::MOVE_LEFT: MoveInventoryCursor(p, false); break;
		case Game::MOVE_RIGHT:MoveInventoryCursor(p, true);  break;
		case Game::MOVE_ENTER: case Game::MOVE_ENTER2: {
			int total = p.inventoryPrediction.Sum();
			if(total <= g.maxInventory) {
				p.inventory.Copy(p.inventoryPrediction);
				p.SetUIState<HandManage>(g,p);
			}
		}	break;
	}
}

// template<typename T>
// bool Player::IsState(const Player& p) {
// 	return p.uistate != NULL && dynamic_cast<T*>(p.uistate) != NULL;
// }

// template <typename T>
// void Player::SetUIState(Game& g, Player& p) {
// 	bool oldState = Player::IsState<T>(p);
// 	if(oldState) { return; }
// 	PlayerState* next = NEWMEM(T);
// 	if(p.uistate != NULL) {
// 		p.uistate->Release();
// 		DELMEM(p.uistate);
// 	}
// 	p.uistate = next;
// 	next->Init(GamePlayer(&g, &p));
// }

void Player::UpdateInput(Game& g, Player& p, int move) {
	if(p.uistate == NULL) {
		p.Init(g);
		Player::SetUIState<HandManage>(g,p);
	}
	p.uistate->ProcessInput(move);
}

void Player::PrintHand (Game& g, Coord pos, int count, Player& p) {
	CLI::resetColor();
	int limit = p.hand.Count () + p.played.Count ();
	int extraSpaces = count - (limit - p.handOffset);
	if (extraSpaces < 0) {
		limit += extraSpaces;
	}
	int bg = CLI::COLOR::DARK_GRAY;
	for (int i = p.handOffset; i < limit; ++i) {
		CLI::move (pos.y + i - p.handOffset, pos.x);
		bool isplayed = i >= p.hand.Count ();
		if (Player::IsState<HandManage>(p) && i == p.currentRow) {
			p.SetConsoleColor(g);
			CLI::putchar ((!isplayed ? '>' : 'x'));
		} else {
			CLI::putchar (' ');
		}
		bool isSelected = !isplayed && p.selectedMark.GetPtr (i) != NULL;
		// indent selected cards slightly
		if (isSelected) {
			CLI::printf (">");
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
		PlayAction::PrintAction (g, card, bg);
		if (isSelected) {
			int selectedIndex = p.selected.IndexOf (i);
			CLI::resetColor();
			CLI::printf ("%2d", selectedIndex);
		} else {
			if(Player::IsState<HandManage>(p) && i == p.currentRow){
				p.SetConsoleColor(g);
				CLI::putchar(!isplayed?'<':'x');
			} else {
				CLI::putchar(' ');
			}
			CLI::resetColor();
			CLI::printf (CanAfford (g, card->input, p.inventory)?" .":"  ");
		}
	}
	if(extraSpaces > 0) {
		CLI::setColor(p.bcolor, -1);
		for (int i = 0; i < extraSpaces; ++i) {
			CLI::move (pos.y + limit + i - p.handOffset, pos.x);
			CLI::printf ("..............");
		}
	}
	CLI::resetColor();
}

void Player::PrintInventory(Game& g, const Player& p, int background, int numberWidth, bool showZeros, List<int> & inventory, const VList<const ResourceType*>& collectableResources, Coord pos, int selected) {
	CLI::move (pos.y, pos.x);
	int fcolor = CLI::COLOR::LIGHT_GRAY;
	CLI::setColor (fcolor, background);
	char formatBuffer[10];
	sprintf(formatBuffer, "%%%dd", numberWidth);
	for (int i = 0; i < inventory.Length (); ++i) {
		if(i == selected) {
			if(Player::IsState<ResWaste>(p)){p.SetConsoleColorBlink();}else{p.SetConsoleColor(g);}
			CLI::putchar('>');CLI::setColor (fcolor, background);
		} else { CLI::putchar(' '); }
		CLI::setColor (collectableResources[i]->color);
		if(showZeros || inventory[i] != 0) {
			CLI::printf (formatBuffer, inventory[i]);
			if(i == selected) {
				if(Player::IsState<ResWaste>(p)){p.SetConsoleColorBlink();}else{p.SetConsoleColor(g);}
				CLI::putchar('<');CLI::setColor(fcolor, background);
			} else { CLI::printf ("%c", collectableResources[i]->icon); }
		} else {
			for(int c=0;c<numberWidth;++c) { CLI::putchar(' '); }
			if(i == selected) {
				if(Player::IsState<ResWaste>(p)){p.SetConsoleColorBlink();}else{p.SetConsoleColor(g);}
				CLI::putchar('<');CLI::setColor(fcolor, background);
			} else { CLI::putchar(' '); }
		}
	}
}

void Player::PrintResourcesInventory(Game& g, Coord cursor, Player& p){
	const int numberWidth = 2;
	CLI::resetColor();
	if(p.validPrediction == PredictionState::valid || p.validPrediction == PredictionState::invalid) {
		PrintInventory(g, p, CLI::COLOR::DARK_GRAY, numberWidth, true, p.inventory, g.collectableResources, cursor, 
			(Player::IsState<ResWaste>(p))?p.inventoryCursor:-1); cursor.y+=1;
		// draw fake resources... or warning message
		PrintInventory(g, p, p.validPrediction?CLI::COLOR::BLACK:CLI::COLOR::RED, numberWidth, true, p.inventoryPrediction, g.collectableResources, cursor, 
			(Player::IsState<ResWaste>(p))?p.inventoryCursor:-1);
	} else {
		if(Player::IsState<ResWaste>(p) || Player::IsState<ResUpgrade>(p)) {
			// if modifying the inventory, show the prediction, which is the modifying list
			PrintInventory(g, p, CLI::COLOR::DARK_GRAY, numberWidth, true, p.inventoryPrediction, g.collectableResources, cursor, p.inventoryCursor);
		} else {
			PrintInventory(g, p, CLI::COLOR::DARK_GRAY, numberWidth, true, p.inventory, g.collectableResources, cursor, -1);
		}
		cursor.y+=1;
		CLI::move(cursor);
		if(p.upgradeChoices == 0) {
			int total = p.inventoryPrediction.Sum();
			CLI::setColor(CLI::COLOR::WHITE, (total<=g.maxInventory)?CLI::COLOR::BLACK:CLI::COLOR::RED);
			CLI::printf("resources:%3d/%2d", total, g.maxInventory);
		} else {
			CLI::setColor(CLI::COLOR::WHITE, (p.upgradesMade<p.upgradeChoices)?CLI::COLOR::RED:CLI::COLOR::BLACK);
			CLI::printf("+ upgrades:%2d/%2d", p.upgradesMade, p.upgradeChoices);
		}
	}
}

void printspaces(int x) {
	for(int i=0;i<x;++i){CLI::putchar(' ');}
}

void Player::PrintUserState(Game& g, Coord cursor, const Player & p) {
	int MAXWIDTH = 19;
	CLI::move (cursor);
	if(g.GetCurrentPlayer()==&p) { p.SetConsoleColor(g); }
	else{CLI::resetColor();}
	CLI::printf ("%.*s", MAXWIDTH, p.name.c_str());
	printspaces(MAXWIDTH-p.name.length());
	CLI::resetColor();
	cursor.y ++; CLI::move (cursor); 
	std::string output = "...";
	if(p.uistate != NULL) { output = p.uistate->GetName(); }
	CLI::printf(output.c_str());
	if(output.length() < MAXWIDTH) { printspaces(MAXWIDTH-output.length()); }
	cursor.y --;
	cursor.x += MAXWIDTH-1;
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

void Player::FinishTurn(Game& g, Player& p) {
	g.NextTurn();
	p.inventoryPrediction.Copy(p.inventory);
	int totalResources = p.inventory.Sum();
	if(totalResources > g.maxInventory) {
		p.SetUIState<ResWaste>(g,p);
	}
}