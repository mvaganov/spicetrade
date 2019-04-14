#include "platform_conio.h"
#include "list.h"
#include "cards.h"

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

class Game
{

};