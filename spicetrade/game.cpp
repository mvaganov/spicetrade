#include "game.h"

void Game::Draw() {
	for(int i = 0; i < GetPlayerCount(); ++i) {
		int column = (i*20);
		Player& p = *GetPlayer(i);
		Player::PrintHand (*this, Coord(column+1,9), handDisplayCount, p);
		Player::PrintResourcesInventory(Coord(column,20), p, *this);
		Player::PrintUserState(Coord(column,23), p, *this);
	}
	Game::PrintAchievements(*this, Coord(0,2));
	Game::PrintMarket(*this, Coord(0,6));
}
void Game::RefreshInput() {
	if(CLI::kbhit()){
		CLI::setColor(CLI::COLOR::LIGHT_GRAY, -1); // reset color
		CLI::move(0,0);
		SetInput(CLI::getch ());
	} else { CLI::sleep(50); }
}

void Game::UpdateObjectiveBuy(Game&g, Player& p, int userInput) {
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
	case Game::MOVE_ENTER: case Game::MOVE_ENTER2: 
		if(g.GetCurrentPlayer() == &p) {
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

void GainSelectedMarketCard(Game& g, Player& p) {
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

void Game::UpdateAcquireMarket(Player& p, Game& g, int userInput) {
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
		GainSelectedMarketCard(g, p);
	}
}

void Game::UpdateMarket(Player& p, int userInput, Game& g) {
	switch(userInput) {
	case Game::MOVE_LEFT:
		p.marketCursor--; if(p.marketCursor < 0) { p.marketCursor = 0; }
		break;
	case Game::MOVE_RIGHT:
		p.marketCursor++; if(p.marketCursor >= g.market.Length()) { p.marketCursor = g.market.Length()-1; }
		break;
	case Game::MOVE_UP:
		p.ui = UserControl::ui_objectives;
		break;
	case Game::MOVE_DOWN:
		p.ui = UserControl::ui_hand;
		break;
	case Game::MOVE_ENTER: case Game::MOVE_ENTER2: {
		if(g.GetCurrentPlayer() == &p){
			if(p.marketCardToBuy == 0) {
				GainSelectedMarketCard(g, p);
			} else {
				int total = p.inventory.Sum();
				if(total < p.marketCursor || g.market[p.marketCursor] == NULL) {
					printf("too expensive, cannot afford.\n");
				} else {
					g.resourcePutInto.SetAll(-1);
					p.marketCardToBuy = p.marketCursor;
					p.marketCursor = 0;
					for(int i = 0; i < p.marketCardToBuy; ++i) {
						if(g.acquireBonus[i] == NULL) {
							g.acquireBonus[i] = NEWMEM(List<int>(p.inventory.Length(), 0));
						}
					}
					p.ui = ui_acquire;
				}
			}
		}
	}
}
}

void Game::PrintAchievements(Game& g, Coord cursor) {
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
		if(p){p->SetConsoleColor(g);}
		putchar((p)?'>':' ');
		const Objective* o = g.achievements[i];
		Objective::PrintObjective(g, o, CLI::COLOR::BLACK);
		CLI::setColor(CLI::COLOR::WHITE, -1);
		if(p){p->SetConsoleColor(g);}
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
			CLI::resetColor(); CLI::printf("     ");
		}
		cursor.y--;
		cursor.x += 14;
	}
}

void Game::PrintMarket(Game& g, Coord cursor) {
	const Player* currentPlayer = g.playerUIOrder[0];
	int totalResources = currentPlayer->inventory.Sum();
	for(int i = 0; i < g.market.Length(); ++i) {
		CLI::move(cursor);
		CLI::setColor(CLI::COLOR::WHITE, -1);
		const Player* p = NULL;
		for(int pi = 0; pi < g.playerUIOrder.Length(); ++pi) {
			if(g.playerUIOrder[pi]->marketCursor == i 
			&&(g.playerUIOrder[pi]->ui == UserControl::ui_cards || g.playerUIOrder[pi]->ui == UserControl::ui_acquire)) {
				p = g.playerUIOrder[pi];
				break;
			}
		}
		if(p){p->SetConsoleColor(g);}
		putchar((p)?'>':' ');
		PlayAction::PrintAction(g, g.market[i], (totalResources >= i)?CLI::COLOR::DARK_GRAY:CLI::COLOR::BLACK);
		CLI::setColor(CLI::COLOR::WHITE, -1);
		if(p){p->SetConsoleColor(g);}
		putchar((p)?'<':' ');
		cursor.y++;
		CLI::move(cursor);
		if(g.acquireBonus[i] != NULL) {
			CLI::setColor(CLI::COLOR::WHITE, -1);
			int background = -1;
			if(currentPlayer && i < currentPlayer->marketCardToBuy && g.resourcePutInto[i] == -1) {
				background = CLI::COLOR::RED;
			}
			Player::PrintInventory(*p, g, background, 1, (p != NULL && currentPlayer->ui == UserControl::ui_acquire), *(g.acquireBonus[i]), 
				g.collectableResources, cursor, 
				(p && i < currentPlayer->marketCardToBuy)?currentPlayer->inventoryCursor:-1);
			CLI::setColor(CLI::COLOR::WHITE, -1);
		} else {
			CLI::resetColor();
			// if(marketCursor == i) {
			// 	CLI::printf(" -- -- -- -- ");
			// } else {
				CLI::printf("             ");
			// }
		}
		cursor.y--;
		cursor.x += 13;
	}
}
