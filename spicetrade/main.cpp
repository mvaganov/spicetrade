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
// [play PlayAction from hand] - state_hand - select PlayAction to play (card)
// [re-order PlayAction hand] - state_hand - during re-order, resource display changes to temp view
// [acquire PlayAction] - state_acquire - select a card from the market. (gain resources on top of card if it has any)
// [acquire deep PlayAction] - state_aquire_pay - pay for card if it isn't first first one.
// [select Objective] - state_objective - purchase objective if resources are there.
// [upgrade action] - state_upgrade - select X resources to upgrade
// [rest action] - put played cards back into hand

enum UserControl {ui_hand, ui_inventory, ui_cards, ui_objectives};

void print (std::string str) { CLI::printf ("%s", str.c_str ()); }
void print (const char* str) { CLI::printf ("%s", str); }

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
	CLI::putchar ('>');
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

bool SubtractResources (const PlayAction* card, List<int>& inventory) {
	std::string input = card->input;
	if (input == "reset") {
		return true;
	}
	for (int i = 0; i < input.length (); ++i) {
		char c = input[i];
		if (c == '.') {

		} else {
			int* index = resourceIndex.GetPtr (c);
			if (index == NULL) {
				printf ("can't afford imaginary resource %c\n", c);
			}
			inventory[*index]--;
		}
	}
}

bool InventoryValid(const List<int>& inventory) {
	bool valid = true;
	for (int i = 0; i < inventory.Length (); ++i) {
		if (inventory.GetAt(i) < 0) { valid = false; }
	}
	return valid;
}

bool CanPlay (const PlayAction* toPlay, List<int>& inventory) {
	List<int> testInventory (inventory); // TODO turn this into a static array and memcpy inventory?
	SubtractResources (toPlay, testInventory);
	return InventoryValid(testInventory);
}

void AddResources (const PlayAction* card, List<int>& inventory, VList<const PlayAction*>& hand, VList<const PlayAction*>& played) {
	std::string output = card->output;
	if (output == "cards") {
		hand.Insert (hand.Count (), played.GetData (), played.Count ());
		played.Clear ();
	} else {
		for (int i = 0; i < output.length (); ++i) {
			char c = output[i];
			if (c == '+') {
				// TODO upgrade choice UI
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

bool Calculate(VList<const PlayAction*>& hand, VList<const PlayAction*>& played,
	VList<int>& selected, List<int>& prediction) {
	bool canAfford = true;
	for(int i = 0; i < selected.Count(); ++i) {
		const PlayAction* card = hand[selected[i]];
		SubtractResources(card, prediction);
		AddResources(card, prediction, hand, played);
		bool canDoIt = InventoryValid(prediction);
		if(!canDoIt) { canAfford = false; }
	}
	return canAfford;
}

void UpdateHand (UserControl& ui, int userInput, int& start, int count, int& currentRow,
				VList<const PlayAction*>& hand,   VList<const PlayAction*>& predictionHand,
				VList<const PlayAction*>& played, VList<const PlayAction*>& predictionPlayed,
				Dictionary<int, int>& selectedMark,
				VList<int>& selected,
				List<int>& inventory, List<int>& prediction,
				bool& valid) {
	switch (userInput) {
	case 'w':
		currentRow--;
		if (currentRow <= start) {
			start += currentRow - start;
		}
		if (start < 0) {
			start = 0;
		}
		if (currentRow < 0) {
			currentRow = 0;
			ui = UserControl::ui_cards;
		}
		break;
	case 's':
		currentRow++;
		if (currentRow >= start + count - 1) {
			start += currentRow - (start + count) + 1;
		}
		if(start+count > predictionHand.Count () + predictionPlayed.Count ()) {
			start = predictionHand.Count () + predictionPlayed.Count () - count;
		}
		if (currentRow >= predictionHand.Count () + predictionPlayed.Count ()) {
			currentRow = predictionHand.Count () + predictionPlayed.Count () - 1;
			ui = UserControl::ui_inventory;
		}
		break;
	case 'd': {
		int* isSelected = selectedMark.GetPtr (currentRow);
		if (isSelected == NULL) {
			if(currentRow < predictionHand.Count()){
				selected.Add (currentRow);
				selectedMark.Set (currentRow, 1);
				prediction.Copy(inventory);
				valid = Calculate(predictionHand, predictionPlayed, selected, prediction);
			}
		}
	} break;
	case 'a': {
		int* isSelected = selectedMark.GetPtr (currentRow);
		if (isSelected != NULL) {
			int sindex = selected.IndexOf (currentRow);
			selected.RemoveAt (sindex);
			const PlayAction* card = hand[currentRow];
			selectedMark.Remove (currentRow);
			if(card->input == "reset" || selected.Count() == 0){
				predictionHand.Copy(hand);
				predictionPlayed.Copy(played);
				// TODO remove cards from unplayed (index > hand.Count()) from selected and selectedMark
			}
			prediction.Copy(inventory);
			valid = Calculate(predictionHand, predictionPlayed, selected, prediction);
		}
	} break;
	case '\n':
	case '\r':
		if (selected.Count () > 0) {
			if (currentRow >= 0 && currentRow <= hand.Count ()) {
				// remove the selected from the hand into a moving list, ordered by selected
				VList<const PlayAction*> reordered;
				for (int i = 0; i < selected.Count (); ++i) {
					int index = selected[i];
					if(index < hand.Count()) {
						reordered.Add (hand[index]);
					} // don't insert played cards
				}
				selected.Sort ();
				for (int i = selected.Count () - 1; i >= 0; --i) {
					int index = selected[i];
					if(index < hand.Count()) {
						hand.RemoveAt (index);
						if (selected[i] < currentRow)
							currentRow--;
					 } // don't insert played cards
				}
				// clear selected
				selected.Clear ();
				selectedMark.Clear ();
				valid = true;
				// insert element into the list at the given index
				hand.Insert (currentRow, reordered.GetData (), reordered.Count ());
				reordered.Clear ();
				prediction.Copy(inventory);
				predictionHand.Copy(hand);
				predictionPlayed.Copy(played);
			}
		} else {
			printf("row %d   \n",currentRow);
			if (currentRow >= 0 && currentRow < hand.Count ()) {
				const PlayAction* toPlay = hand[currentRow];
				printf("   %s   \n",toPlay->input.c_str());
				bool canDoIt = CanPlay (toPlay, inventory);
				if (canDoIt) {
					SubtractResources (toPlay, inventory);
					AddResources (toPlay, inventory, hand, played);
					hand.RemoveAt (currentRow);
					if (toPlay->output == "cards") {
						hand.Add (toPlay);
					} else {
						played.Add (toPlay);
					}
					prediction.Copy(inventory);
					predictionHand.Copy(hand);
					predictionPlayed.Copy(played);
				}
			}
		}
	}
}


void PrintHand (UserControl& ui, VList<const PlayAction*>& hand, VList<const PlayAction*>& played,
				int start, int count, int currentRow,
				Dictionary<int, int>& selectedMark, VList<int>& selected, 
				List<int>& inventory, Coord pos) {
	int limit = hand.Count () + played.Count ();
	int extraSpaces = count - (limit - start);
	if (extraSpaces < 0) {
		limit += extraSpaces;
	}
	int bg = CLI::COLOR::DARK_GRAY;
	for (int i = start; i < limit; ++i) {
		CLI::move (pos.y + i - start, pos.x);
		bool isplayed = i >= hand.Count ();
		if (ui == UserControl::ui_hand && i == currentRow) {
			print ((!isplayed ? ">" : "x"));
		} else {
			print (" ");
		}
		bool isSelected = !isplayed && selectedMark.GetPtr (i) != NULL;
		// indent selected cards slightly
		if (isSelected) {
			print (" ");
		}
		const PlayAction* card = NULL;
		int bg = CLI::COLOR::BLACK;
		if (i < hand.Count ()) {
			card = hand[i];
			bg = CLI::COLOR::DARK_GRAY;
		} else if (i < hand.Count () + played.Count ()) {
			card = played[i - hand.Count ()];
			bg = CLI::COLOR::BLACK;
		}
		PrintAction (card, bg);
		if (isSelected) {
			int selectedIndex = selected.IndexOf (i);
			CLI::printf ("%2d", selectedIndex);
		} else {
			if (CanPlay (card, inventory)) {
				print ("  .");
			} else {
				print ("   ");
			}
		}
	}
	CLI::setColor (CLI::COLOR::GREEN);
	for (int i = 0; i < extraSpaces; ++i) {
		CLI::move (pos.y + 1 + limit + i - start, pos.x);
		CLI::printf ("..............");
	}
	CLI::setColor (-1, -1);
}

void PrintInventory(int background, List<int> & inventory, VList<const ResourceType*>& collectableResources, Coord pos, int selected) {
	CLI::move (pos.y, pos.x);
	CLI::setColor (CLI::COLOR::LIGHT_GRAY, background);
	for (int i = 0; i < inventory.Length (); ++i) {
		if(i == selected) {
			CLI::setColor (CLI::COLOR::WHITE, background); CLI::putchar('>');
		} else { CLI::putchar(' '); }
		CLI::setColor (collectableResources[i]->color);
		CLI::printf ("%2d", inventory[i]);
		if(i == selected) {
			CLI::setColor (CLI::COLOR::WHITE, background); CLI::putchar('<');
		} else { CLI::printf ("%c", collectableResources[i]->icon); }
	}
}

int clampint(int n, int mn, int mx) { return ((n<mn)?mn:((n>mx)?mx:n)); }

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
	for (int i = 0; i < g_len_play_deck; ++i) {
		actionDeck.Set (g_play_deck[i].input, &(g_play_deck[i]));
		play_deck.Add (&(g_play_deck[i]));
	}

	printf ("%d possible actions\n", actionDeck.Count ());
	//VList<Dictionary<std::string, const PlayAction*>::KVP> kvp(g_len_play_deck);
	//kvp.SetCount(g_len_play_deck);
	//actionDeck.ToArray(kvp.GetData());
	// for(int i = 0; i < g_len_playList; ++i) {
	//     const PlayAction * v = kvp[i].value;
	//     //printf("%s>%s : \"%s\"\n", v->input, v->output, v->name);
	//     printf("%s\n", v->ToString(10).c_str());
	// }

	platform_shuffle (play_deck.GetData (), 0, play_deck.Count ());
	VList<const PlayAction*> hand, predictionHand;
	VList<const PlayAction*> played, predictionPlayed;
	for (int i = 0; i < g_len_playstart; ++i) {
		hand.Add (&(g_playstart[i]));
	}
	for (int i = 0; i < 3; ++i) {
		hand.Add (play_deck.PopLast ());
	}
	for (int i = 0; i < 7; ++i) {
		played.Add (play_deck.PopLast ());
	}
	predictionHand.Copy(hand);
	predictionPlayed.Copy(played);

	CLI::init ();
	CLI::setSize (CLI::getWidth (), 25);
	CLI::fillScreen (' ');
	CLI::move (0, 0);
	CLI::printf ("Hello World!");
	List<int> inventory (collectableResources.Count (), 0);
	inventory[0] = 2; // start with 2 basic resource

	int currentRow = 0;
	Dictionary<int, int> selectedMark;
	VList<int> selected;
	List<int> prediction(inventory);
	bool running = true;
	int start = 0, count = 10;
	Coord cursor;
	int inventoryCursor = 2;
	bool valid = true;
	UserControl ui = UserControl::ui_hand;
	int height = CLI::getHeight();
	int maxInventory = 10;
	while (running) {
		// draw
		cursor.set(1,height-3);
		// draw resources
		if(ui == UserControl::ui_hand) {
			PrintInventory(CLI::COLOR::DARK_GRAY, inventory, collectableResources, cursor, 
				(ui==UserControl::ui_inventory)?inventoryCursor:-1); cursor.y+=1;
			// draw fake resources... or warning message
			PrintInventory(valid?CLI::COLOR::BLACK:CLI::COLOR::RED, prediction, collectableResources, cursor, 
				(ui==UserControl::ui_inventory)?inventoryCursor:-1);
		} else {
			PrintInventory(CLI::COLOR::DARK_GRAY, prediction, collectableResources, cursor, 
				(ui==UserControl::ui_inventory)?inventoryCursor:-1); cursor.y+=1;
			CLI::move(cursor);
			int total = prediction.Sum();
			CLI::setColor(CLI::COLOR::WHITE, (total<=maxInventory)?CLI::COLOR::BLACK:CLI::COLOR::RED);
			printf("resources:%3d/%2d", total, maxInventory);
		}
		cursor.y+=1;
		// draw username
		CLI::move (cursor); CLI::setColor (CLI::COLOR::WHITE, -1); CLI::printf ("Mr. V");
		cursor.y += 1;
		cursor.set(1,height-(count+4));

		PrintHand (ui, predictionHand, predictionPlayed, 
			start, count, currentRow, selectedMark, selected, prediction, cursor);
		// input
		int userInput = CLI::getch ();
		CLI::printf ("%d\n", userInput);
		// update
		switch (userInput) {
		case 'q':
			running = false;
			break;
		default:
			switch(ui) {
			case UserControl::ui_hand:
				UpdateHand (ui, userInput, start, count,  currentRow, 
					hand, predictionHand, played, predictionPlayed, 
					selectedMark, selected, inventory, prediction, valid);
				// on hand exit, clear selected, reset predictions to current state
				break;
			case UserControl::ui_inventory: {
				// on enter, prediction should be a copy of inventory. modify prediction
				switch(userInput) {
					case 'w':
						prediction[inventoryCursor] = clampint(prediction[inventoryCursor]+1,0,inventory[inventoryCursor]);
						break;
					case 'a':
						inventoryCursor--;
						if(inventoryCursor<0) inventoryCursor = 0;
						break;
					case 's':
						prediction[inventoryCursor] = clampint(prediction[inventoryCursor]-1,0,inventory[inventoryCursor]);
						break;
					case 'd':
						inventoryCursor++;
						if(inventoryCursor>=inventory.Length()) inventoryCursor = inventory.Length()-1;
						break;
					case '\n': case '\r':
						ui = UserControl::ui_hand;
						inventory.Copy(prediction);
						break;
				}
			}
				break;
			case UserControl::ui_cards: {
				switch(userInput) {
					case 's':
						ui = UserControl::ui_hand;
						break;
				}
			}
				break;
			case UserControl::ui_objectives:
				break;
			}
			break;
		}
	}
	CLI::release ();

	return 0;
}