#include "game.h"
#include "player.h"


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
		} else {
			printf("row %d   \n", p.currentRow);
			if (p.currentRow >= 0 && p.currentRow < p.hand.Count ()) {
				const PlayAction* toPlay = p.hand[p.currentRow];
				printf("   %s   \n",toPlay->input.c_str());
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

void Player::PrintHand (Game& g, Coord pos, int count, Player& p) {
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
			CLI::printf ("%2d", selectedIndex);
		} else {
			CLI::putchar((p.ui == UserControl::ui_hand && i == p.currentRow)?(!isplayed?'<':'x'):' ');
			if (CanPlay (g, card, p.inventory)) {
				CLI::printf (" .");
			} else {
				CLI::printf ("  ");
			}
		}
	}
	CLI::setColor (CLI::COLOR::GREEN);
	for (int i = 0; i < extraSpaces; ++i) {
		CLI::move (pos.y + limit + i - p.handOffset, pos.x);
		CLI::printf ("..............");
	}
}
