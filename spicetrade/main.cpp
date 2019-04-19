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
// TODO pass the turn
// TODO force resource reconcile (down to max) after turn is passed. require reconcile finish before new turn starts
// TODO re-implement predictions

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
	Game g(3, CLI::getWidth (), 25);
	while (g.running) {
		g.Draw();
		g.RefreshInput();
		g.ProcessInput(); // TODO replace with Update() once statemachine code is implemented
	}
	return 0;
}