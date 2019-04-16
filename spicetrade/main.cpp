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
// [select Objective] - state_objective - purchase objective if resources are there.
// [upgrade action] - state_upgrade - select X resources to upgrade
//x [rest action] - put played cards back into hand

enum UserControl {ui_hand, ui_inventory, ui_cards, ui_acquire, ui_objectives};
enum PredictionState {none, valid, invalid};

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

void RefreshPrediction(PredictionState& valid, VList<int>& selected, List<int>& inventory,
	List<int>& prediction, VList<const PlayAction*>& predictionHand, VList<const PlayAction*>& predictionPlayed){
	prediction.Copy(inventory);
	if(selected.Count() == 0){
		valid = PredictionState::none;
	} else {
		bool canAfford = Calculate(predictionHand, predictionPlayed, selected, prediction);
		valid = canAfford?PredictionState::valid:PredictionState::invalid;
	}
}

void UpdateHand (UserControl& ui, int userInput, int& start, int count, int& currentRow,
				VList<const PlayAction*>& hand,   VList<const PlayAction*>& predictionHand,
				VList<const PlayAction*>& played, VList<const PlayAction*>& predictionPlayed,
				Dictionary<int, int>& selectedMark,
				VList<int>& selected,
				List<int>& inventory, List<int>& prediction,
				PredictionState& valid) {
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
				RefreshPrediction(valid, selected, inventory, prediction, predictionHand, predictionPlayed);
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
			RefreshPrediction(valid, selected, inventory, prediction, predictionHand, predictionPlayed);
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
				valid = PredictionState::none;
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

void PrintInventory(int background, int numberWidth, bool showZeros, List<int> & inventory, VList<const ResourceType*>& collectableResources, Coord pos, int selected) {
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

void PrintResourcesInventory(UserControl ui, Coord cursor, 
	VList<const ResourceType*>& collectableResources, List<int>& inventory, List<int>& prediction, 
	int inventoryCursor, int maxInventory, PredictionState& validPrediction){
	const int numberWidth = 2;
	if(validPrediction == PredictionState::valid || validPrediction == PredictionState::invalid) {
		PrintInventory(CLI::COLOR::DARK_GRAY, numberWidth, true, inventory, collectableResources, cursor, 
			(ui==UserControl::ui_inventory)?inventoryCursor:-1); cursor.y+=1;
		// draw fake resources... or warning message
		PrintInventory(validPrediction?CLI::COLOR::BLACK:CLI::COLOR::RED, numberWidth, true, prediction, collectableResources, cursor, 
			(ui==UserControl::ui_inventory)?inventoryCursor:-1);
	} else {
		if(ui==UserControl::ui_inventory) {
			// if modifying the inventory, show the prediction, which is the modifying list
			PrintInventory(CLI::COLOR::DARK_GRAY, numberWidth, true, prediction, collectableResources, cursor, inventoryCursor);
		} else {
			PrintInventory(CLI::COLOR::DARK_GRAY, numberWidth, true, inventory, collectableResources, cursor, -1);
		}
		cursor.y+=1;
		CLI::move(cursor);
		int total = prediction.Sum();
		CLI::setColor(CLI::COLOR::WHITE, (total<=maxInventory)?CLI::COLOR::BLACK:CLI::COLOR::RED);
		printf("resources:%3d/%2d", total, maxInventory);
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

	platform_shuffle (play_deck.GetData (), 0, play_deck.Count ());
	VList<const PlayAction*> hand, predictionHand;
	VList<const PlayAction*> played, predictionPlayed;

	const int marketCards = 6;
	List<const PlayAction*> market(marketCards);
	List<List<int>*> acquireBonus(marketCards, NULL);
	
	for (int i = 0; i < marketCards; ++i) {
		market.Set(i, play_deck.PopLast ());
	}
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

	// debug
	inventory[1] = 1;
	inventory[2] = 3;

	int currentRow = 0;
	Dictionary<int, int> selectedMark;
	VList<int> selected;
	List<int> prediction(inventory);
	bool running = true;
	int start = 0, count = 10;
	Coord cursor;
	int inventoryCursor = 2;
	int marketCursor = 0;
	int marketCardToBuy = -1;
	int upgradeChoices = 0;
	List<int> resourcePutInto(market.Length()-1, 0); // used during acquire and upgrade UI
	PredictionState validPrediction = PredictionState::none;
	UserControl ui = UserControl::ui_hand;
	int height = CLI::getHeight();
	int maxInventory = 10;
	while (running) {
		// draw
		cursor.set(1,height-3);
		// draw resources
		PrintResourcesInventory(ui, cursor, collectableResources, inventory, prediction,
			inventoryCursor, maxInventory, validPrediction);
		cursor.y+=2;
		// draw username
		CLI::move (cursor); CLI::setColor (CLI::COLOR::WHITE, -1); CLI::printf ("Mr. V");
		cursor.y += 1;
		cursor.set(1,height-(count+4));

		PrintHand (ui, predictionHand, predictionPlayed, 
			start, count, currentRow, selectedMark, selected, prediction, cursor);

		cursor.set(1, 8);
		for(int i = 0; i < market.Length(); ++i) {
			CLI::move(cursor);
			CLI::setColor(CLI::COLOR::WHITE, -1);
			putchar((marketCursor == i)?'>':' ');
			PrintAction(market[i], CLI::COLOR::BLACK);
			CLI::setColor(CLI::COLOR::WHITE, -1);
			putchar((marketCursor == i)?'<':' ');
			cursor.y++;
			CLI::move(cursor);
			if(acquireBonus[i] != NULL) {
				CLI::setColor(CLI::COLOR::WHITE, -1);
				
				int background = -1;
				if(marketCardToBuy != -1 && resourcePutInto[i] == -1) { background = CLI::COLOR::RED; }

				PrintInventory(background, 1, marketCursor==i && marketCardToBuy>=0, *(acquireBonus[i]), collectableResources, cursor, 
					(marketCursor==i && i<marketCardToBuy)?inventoryCursor:-1);
				CLI::setColor(CLI::COLOR::WHITE, -1);

			} else {
				CLI::setColor(CLI::COLOR::LIGHT_GRAY, -1);
				if(marketCursor == i) {
					CLI::printf(" -- -- -- -- ");
				} else {
					CLI::printf("             ");
				}
			}
			cursor.y--;
			cursor.x += 14;
		}

		// input
		CLI::move(0,0);
		int userInput = CLI::getch ();
		// update
		switch (userInput) {
		case 27:
			printf("quitting...                ");
			running = false;
			break;
		default:
			switch(ui) {
			case UserControl::ui_hand:
				printf("hand management        ");
				UpdateHand (ui, userInput, start, count,  currentRow, 
					hand, predictionHand, played, predictionPlayed, 
					selectedMark, selected, inventory, prediction, validPrediction);
				// on hand exit, clear selected, reset predictions to current state
				break;
			case UserControl::ui_inventory: {
				printf("resource management        ");
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
				printf("select cards              ");
				switch(userInput) {
					case 'a':
						marketCursor--; if(marketCursor < 0) { marketCursor = 0; }
						break;
					case 'd':
						marketCursor++; if(marketCursor >= market.Length()) { marketCursor = market.Length()-1; }
						break;
					case 's':
						ui = UserControl::ui_hand;
						break;
					case '\n': case '\r': {
						int total = inventory.Sum();
						if(total < marketCursor) {
							printf("too expensive, cannot afford.\n");
						} else {
							resourcePutInto.SetAll(-1);
							marketCardToBuy = marketCursor;
							marketCursor = 0;
							for(int i = 0; i < marketCardToBuy; ++i) {
								if(acquireBonus[i] == NULL) {
									acquireBonus[i] = NEWMEM(List<int>(inventory.Length(), 0));
								}
							}
							ui = ui_acquire;
						}
					}
						break;
				}
			}	break;
			case UserControl::ui_acquire: {
				printf("acquire card              ");
				bool probablyPaidUp = marketCardToBuy == 0;
				if(!probablyPaidUp) {
					switch(userInput) {
						case 'w':
							if(resourcePutInto[marketCursor] == -1 && inventory[inventoryCursor] > 0) {
								resourcePutInto[marketCursor] = inventoryCursor;
								inventory[resourcePutInto[marketCursor]]--;
								(*acquireBonus[marketCursor])[resourcePutInto[marketCursor]]++;
							}
							break;
						case 'a':
							if(resourcePutInto[marketCursor] == -1) {
								int loopGuard = 0;
								do {
									inventoryCursor--;
									if(inventoryCursor < 0) {
										if(marketCursor > 0) {
											marketCursor--;
										} else {
											marketCursor = marketCardToBuy-1;
										}
										inventoryCursor = inventory.Length()-1;
									}
									if(loopGuard++ > inventory.Length()*marketCardToBuy) {break;}
								} while(inventory[inventoryCursor] == 0);
							} else {
								if(marketCursor > 0) marketCursor--;
							}
							break;
						case 's':
							if(resourcePutInto[marketCursor] != -1) {
								(*acquireBonus[marketCursor])[resourcePutInto[marketCursor]]--;
								inventory[resourcePutInto[marketCursor]]++;
								resourcePutInto[marketCursor] = -1;
							}
							break;
						case 'd':
							if(resourcePutInto[marketCursor] == -1) {
								int loopGuard = 0;
								do {
									inventoryCursor++;
									if(inventoryCursor >= inventory.Length()) {
										if(marketCursor < marketCardToBuy-1) {
											marketCursor++;
										} else {
											marketCursor = 0;
										}
										inventoryCursor = 0;
									}
									if(loopGuard++ > inventory.Length()*marketCardToBuy) {break;}
								} while(inventory[inventoryCursor] == 0);
							} else {
								if(marketCursor < marketCardToBuy-1) marketCursor++;
							}
							break;
						case '\n': case '\r':
							probablyPaidUp = true;
							break;
					}
				}
				if(marketCursor >= 0 && marketCursor < resourcePutInto.Length() && resourcePutInto[marketCursor] != -1) {
					inventoryCursor = resourcePutInto[marketCursor];
				}

				bool paidUp = false;
				if(probablyPaidUp) {
					// check if all of the market positions before this one are paid
					paidUp = true;
					for(int i = 0; i < resourcePutInto.Length() && i < marketCardToBuy; ++i) {
						if(resourcePutInto[i] == -1) { paidUp = false; break; }
					}
				}
				if(paidUp) {
					List<int>* bonus = acquireBonus[marketCardToBuy];
					hand.Insert(0, market[marketCardToBuy]);
					predictionHand.Copy(hand);
					market.Shift(marketCardToBuy);
					if(play_deck.Count() > 0) {
						market[market.Length()-1] = play_deck.PopLast();
					} else {
						market[market.Length()-1] = NULL;
					}
					if(bonus != NULL) {
						for(int i = 0; i < inventory.Length(); ++i) {
							inventory[i] += (*bonus)[i];
						}
						DELMEM(bonus);
					}
					acquireBonus.Shift(marketCardToBuy);
					acquireBonus[acquireBonus.Length()-1] = NULL;
					marketCardToBuy = -1;
					ui = UserControl::ui_cards;
				}
			}	break;
			case UserControl::ui_objectives:
				break;
			}
			break;
		}
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