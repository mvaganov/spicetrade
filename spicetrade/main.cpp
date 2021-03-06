#include "cli.h"
#include "dictionary.h"
#include "game.h"
#include "platform_conio.h"
#include "platform_random.h"
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

// TODO re-implement predictions
// x re-implement double-buffering to prevent flickering
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
// TODO finish refactoring CLI... strip-out redundant platform_ code, merge BufferManager and CLIBuffer (?)

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

void printRBT(RBT* tree) {
	auto iter = tree->createIter ();
	int i = 0, count = tree->size();
	printf("["); fflush(stdout);
	for (void* v = iter->first (); i < count; v = iter->next (), i++) {
		printf("%3zi", (size_t)v); fflush(stdout);
	}
	printf(" ]\n"); fflush(stdout);
	iter->dealloc ();
}

int originalSeed = 
	//5831 //134217727
	//22   //67108863
	//2    //67108863
	//5    //134217727
	//17   //134217727
	//63   //134217727
	//709  //134217727
	52711;
int seed = originalSeed;
int rand() {
	seed *= 5831;
	return seed;
}
#include <vector>
int main (int argc, const char** argv) {
	// for(int i = 0; i < (1<<30); ++i) {
	// 	int n = rand();
	// 	if(n == originalSeed) {
	// 		printf("periodocity at %d     %d\n",i,seed);
	// 		break;
	// 	}
	// 	if(i% (1<<20) == 0){
	// 		printf("%d  %d\n", i,seed);
	// 	}
	// }
	// platform_getch();

	// RBT* rbt = RBT::create(NULL);
	// VList<int> list;
	// printf("   ");
	// for(int i = 0; i < 20; ++i) {
	// 	int n = platform_random() % 100;
	// 	list.Add(n);
	// 	rbt->insert((void*)(size_t)n);
	// 	printf("%3zi", (size_t)n);
	// }
	// printf("\n");

	// for(int i = 0; i < list.Count(); ++i) {
	// 	//rbt->printTree();
	// 	printf("%i", list[i]);
	// 	printRBT(rbt);
	// 	rbt->remove((void*)(size_t)list[i]);
	// }

	int numPlayers=2;
	// std::cout << "How many players? ";
	// std::cin >> numPlayers;
	Game g;
	g.Init(numPlayers);
//	MEM::REPORT_MEMORY();
	while (g.IsRunning()) {
		g.Draw();
		g.RefreshInput();
		g.Update();
	}
	return 0;
}