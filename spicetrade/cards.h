#pragma once
#include "platform_conio.h"
#include "resource.h"
#include "objective.h"
#include "tradecard.h"
// Raw Resources, Labor, Goods, Efficacy
extern const ResourceType * g_resources;
extern const int g_len_resources;

// how the game is won --with legacy projects
extern const Objective * g_objective;
extern const int g_len_objective;

// what actions the players can get during the game
extern const PlayAction * g_play_deck;
extern const int g_len_play_deck;

// what actions the players always start with
extern const PlayAction * g_playstart;
extern const int g_len_playstart;
