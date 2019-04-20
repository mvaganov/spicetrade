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

// TODO CLI double-buffering with dirty-characters (to prevent flickering and cursor blinking)
// TODO re-implement predictions
// TODO fix upgrade UI during predictions
// TODO list actions that have happened in a history list:
//   - card played (including pre and post resources)
//   - cards refreshed (including pre played list)
//   - market card acquired (including what slot, and payment for card if needed)
//   - objective aqcuired (including pre and post resources)
//   - resources reconciled (including pre and post resources)
//   - upgrade made (including which upgrade card was used, pre and post resources)
// TODO undo the last action on the history list if the current player presses cancel
// TODO allow player to change their name and colors (fore and back colors must be different)

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

int main (int argc, const char** argv) {
	Game g(1, CLI::getWidth (), 25);
	while (g.IsRunning()) {
		g.Draw();
		g.RefreshInput();
		g.Update();
	}
	return 0;
}