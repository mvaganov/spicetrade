#include "game.h"
#include "player.h"

void Player::SetConsoleColor(Game& g) const {
	if(g.GetCurrentPlayer() != this) {
		CLI::setColor(fcolor,bcolor);
	} else {
		if((CLI::upTimeMS() & (1 << 9)) == 0){
			CLI::setColor(bcolor,fcolor);
		} else {
			CLI::setColor(fcolor,bcolor);
		}
	}
}

bool Player::CanPlay (Game& g, const PlayAction* toPlay, List<int>& inventory) {
	if(toPlay == NULL) return false;
	List<int> testInventory (inventory); // TODO turn this into a static array and memcpy inventory?
	SubtractResources (g, toPlay, testInventory);
	return InventoryValid(testInventory);
}

bool Player::SubtractResources (Game& g, const PlayAction* card, List<int>& inventory) {
	if(card == NULL) return false;
	std::string input = card->input;
	if (input == "reset") {
		return true;
	}
	bool valid = true;
	for (int i = 0; i < input.length (); ++i) {
		char c = input[i];
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

void Player::AddResources (Game& g, int& upgradesToDo, const PlayAction* card, List<int>& inventory, VList<const PlayAction*>& hand, VList<const PlayAction*>& played) {
	if(card == NULL) return;
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
				int* index = g.resourceIndex.GetPtr (c);
				if (index == NULL) {
					printf ("no such resource %c\n", c);
				} // <-- here comes the crash
				inventory[*index]++;
			}
		}
	}
}

bool Player::Calculate(Game& g, int& upgradesToDo, VList<const PlayAction*>& hand, VList<const PlayAction*>& played,
	VList<int>& selected, List<int>& prediction) {
	bool canAfford = true;
	for(int i = 0; i < selected.Count(); ++i) {
		const PlayAction* card = hand[selected[i]];
		SubtractResources(g, card, prediction);
		AddResources(g, upgradesToDo, card, prediction, hand, played);
		bool canDoIt = InventoryValid(prediction);
		if(!canDoIt) { canAfford = false; }
	}
	return canAfford;
}

void Player::RefreshPrediction(Game& g, Player& p) {
	p.inventoryPrediction.Copy(p.inventory);
	if(p.selected.Count() == 0){
		p.validPrediction = PredictionState::none;
	} else {
		bool canAfford = Player::Calculate(g, p.upgradeChoices, p.handPrediction, p.playedPrediction, p.selected, p.inventoryPrediction);
		p.validPrediction = canAfford?PredictionState::valid:PredictionState::invalid;
	}
}

void Player::UpdateUpgrade(Player& p, int userInput) {
	if(p.ui != p.lastState) {
		p.inventoryPrediction.Copy(p.inventory);
		p.upgradesMade = 0;
	}
	switch(userInput) {
	case Game::MOVE_UP:
		if(p.upgradesMade < p.upgradeChoices && p.inventoryCursor < p.inventory.Length()-1 
		&& p.inventoryPrediction[p.inventoryCursor] > 0) {
			p.inventoryPrediction[p.inventoryCursor]--;
			p.inventoryPrediction[p.inventoryCursor+1]++;
			p.upgradesMade++;
		}
		break;
	case Game::MOVE_DOWN:
		if(p.upgradesMade > 0 && p.inventoryCursor > 0 
		&& p.inventoryPrediction[p.inventoryCursor] > 0
		&& p.inventory[p.inventoryCursor] < p.inventoryPrediction[p.inventoryCursor]) {
			p.inventoryPrediction[p.inventoryCursor]--;
			p.inventoryPrediction[p.inventoryCursor-1]++;
			p.upgradesMade--;
		}
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
		p.upgradeChoices = 0;
		p.upgradesMade = 0;
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
			p.ui = UserControl::ui_cards;
		}
		break;
	case Game::MOVE_DOWN:
		p.currentRow++;
		if (p.currentRow >= p.handOffset + count - 1) {
			p.handOffset += p.currentRow - (p.handOffset + count) + 1;
		}
		if(p.handOffset+count > p.handPrediction.Count () + p.playedPrediction.Count ()) {
			p.handOffset = p.handPrediction.Count () + p.playedPrediction.Count () - count;
		}
		if (p.currentRow >= p.handPrediction.Count () + p.playedPrediction.Count ()) {
			p.currentRow = p.handPrediction.Count () + p.playedPrediction.Count () - 1;
			//ui = UserControl::ui_inventory;
		}
		if(p.handOffset < 0) { p.handOffset = 0; }
		break;
	case Game::MOVE_RIGHT: {
		printf("prediction algorithms disabled\n"); break;
		int* isSelected = p.selectedMark.GetPtr (p.currentRow);
		if (isSelected == NULL) {
			if(p.currentRow < p.handPrediction.Count()){
				p.selected.Add (p.currentRow);
				p.selectedMark.Set (p.currentRow, 1);
				RefreshPrediction(g, p);
				if(p.upgradeChoices != 0) { p.ui = UserControl::ui_upgrade; } // TODO add code to make the inventory numbers show up, and the upgrade message too.
			}
		}
	} break;
	case Game::MOVE_LEFT: {
		printf("prediction algorithms disabled\n"); break;
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
				bool canDoIt = CanPlay (g, toPlay, p.inventory);
				if (canDoIt) {
					SubtractResources (g, toPlay, p.inventory);
					AddResources (g, p.upgradeChoices, toPlay, p.inventory, p.hand, p.played);
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

int clampint(int n, int mn, int mx) { return ((n<mn)?mn:((n>mx)?mx:n)); }
void Player::UpdateInventory(Player& p, int userInput) {
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

void Player::UpdateInput(Player& p, Game& g, int move) {
	switch(p.ui) {
	case UserControl::ui_hand:
		p.uimode = "  card management  ";
		Player::UpdateHand (g, p, move, g.handDisplayCount);
		// on hand exit, clear p.selected, reset predictions to current state
		break;
	case UserControl::ui_inventory:
		p.uimode = "resource management";
		// on enter, prediction should be a copy of inventory. modify prediction
		Player::UpdateInventory(p, move);
		break;
	case UserControl::ui_cards:
		p.uimode = "selecting next card";
		printf("select cards              ");
		Game::UpdateMarket(p, move, g);
		break;
	case UserControl::ui_acquire:
		p.uimode = "acquiring next card";
		printf("acquire card              ");
		Game::UpdateAcquireMarket(p, g, move);
		break;
	case UserControl::ui_upgrade:
		p.uimode = "upgrading resources";
		Player::UpdateUpgrade(p, move);
		break;
	case UserControl::ui_objectives:
		p.uimode = "selecting objective";
		Game::UpdateObjectiveBuy(g, p, move);
		break;
	}
	p.lastState = p.ui;
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
		if (p.ui == UserControl::ui_hand && i == p.currentRow) {
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
			if(p.ui == UserControl::ui_hand && i == p.currentRow){
				p.SetConsoleColor(g);
				CLI::putchar(!isplayed?'<':'x');
			} else {
				CLI::putchar(' ');
			}
			CLI::resetColor();
			if (CanPlay (g, card, p.inventory)) {
				CLI::printf (" .");
			} else {
				CLI::printf ("  ");
			}
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

void Player::PrintInventory(const Player& p, Game& g,int background, int numberWidth, bool showZeros, List<int> & inventory, const VList<const ResourceType*>& collectableResources, Coord pos, int selected) {
	CLI::move (pos.y, pos.x);
	CLI::setColor (CLI::COLOR::LIGHT_GRAY, background);
	char formatBuffer[10];
	sprintf(formatBuffer, "%%%dd", numberWidth);
	for (int i = 0; i < inventory.Length (); ++i) {
		if(i == selected) {
			p.SetConsoleColor(g);//CLI::setColor (CLI::COLOR::WHITE, background);
			CLI::putchar('>');
		} else { CLI::putchar(' '); }
		CLI::setColor (collectableResources[i]->color);
		if(showZeros || inventory[i] != 0) {
			CLI::printf (formatBuffer, inventory[i]);
			if(i == selected) {
				p.SetConsoleColor(g);CLI::putchar('<');
			} else { CLI::printf ("%c", collectableResources[i]->icon); }
		} else {
			for(int c=0;c<numberWidth;++c) { CLI::putchar(' '); }
			if(i == selected) {
				p.SetConsoleColor(g);CLI::putchar('<');
			} else { CLI::putchar(' '); }
		}
	}
}

void Player::PrintResourcesInventory(Coord cursor, Player& p, Game& g){
	const int numberWidth = 2;
	CLI::resetColor();
	if(p.validPrediction == PredictionState::valid || p.validPrediction == PredictionState::invalid) {
		PrintInventory(p, g, CLI::COLOR::DARK_GRAY, numberWidth, true, p.inventory, g.collectableResources, cursor, 
			(p.ui==UserControl::ui_inventory)?p.inventoryCursor:-1); cursor.y+=1;
		// draw fake resources... or warning message
		PrintInventory(p, g, p.validPrediction?CLI::COLOR::BLACK:CLI::COLOR::RED, numberWidth, true, p.inventoryPrediction, g.collectableResources, cursor, 
			(p.ui==UserControl::ui_inventory)?p.inventoryCursor:-1);
	} else {
		if(p.ui==UserControl::ui_inventory || p.ui==UserControl::ui_upgrade) {
			// if modifying the inventory, show the prediction, which is the modifying list
			PrintInventory(p, g, CLI::COLOR::DARK_GRAY, numberWidth, true, p.inventoryPrediction, g.collectableResources, cursor, p.inventoryCursor);
		} else {
			PrintInventory(p, g, CLI::COLOR::DARK_GRAY, numberWidth, true, p.inventory, g.collectableResources, cursor, -1);
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

void Player::PrintUserState(Coord cursor, const Player & p) {
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