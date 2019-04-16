#include "cards.h"

// Raw Resources, Labor, Goods, Efficacy
	const ResourceType resources[] = {
	ResourceType("Resource",        '#', 1, PLATFORM_COLOR_RED | PLATFORM_COLOR_GREEN | PLATFORM_COLOR_INTENSITY),
	ResourceType("Labor",           '&', 1, PLATFORM_COLOR_RED | PLATFORM_COLOR_INTENSITY),
	ResourceType("Goods",           '@', 1, PLATFORM_COLOR_GREEN | PLATFORM_COLOR_INTENSITY),
	ResourceType("Knowledge",       '%', 1, PLATFORM_COLOR_BLUE | PLATFORM_COLOR_GREEN | PLATFORM_COLOR_INTENSITY),
	ResourceType("Anything",        '.', 0, PLATFORM_COLOR_RED | PLATFORM_COLOR_GREEN | PLATFORM_COLOR_BLUE),
	ResourceType("Something better",'+', 0, PLATFORM_COLOR_RED | PLATFORM_COLOR_GREEN | PLATFORM_COLOR_BLUE | PLATFORM_COLOR_INTENSITY),
};
// # requires resources, which a single person could conceivably provide
// ## requires significant resources, which a group of people could provide
// ### requires large-scale collection, far beyond the limits of one person or a small group
// #### requires collection at a large community scale
// ##### requires resources from a giving from a society
// @ requires a specialized tool for a job
// @@ requires an expert tool
// @@@ requires an extremely precise and high-quality tool
// @@@@ requires specialized hard-to-develop tools, customized for a specific task
// @@@@@ requires epic tool, whose existance redefines what is possible (internet, transistor, telephone, factories, steam engine, )
// & requires work of one person working dilligently
// && requires work of a small group of people, some maintenance expected
// &&& requires work of a large group of people, with hierarchy implied in the scale
// &&&& requires work of a community, which must have a self-perpetuating culture
// &&&&& requires collective will of society changing itself
// % requires special knowledge or research
// %% requires more knowledge or research than a single person could posess
// %%% requires more knowledge and deep expertise than a small group could manage
// %%%% requires extremely varied and extremely deep expertise, using highest-technology of the day
// %%%%% requires epic knowledge: redefines the limits of what we think is possible, and by being known, changes society (information technology, radio waves, electricity, chemistry, physics, mass production, the scientific method)
const Objective objective[] = {
	Objective("##&&",  6,  "meeting space"),
	Objective("###&&", 7,  "common supplies"),
	Objective("&&&&",  8,  "company meeting"),
	Objective("##@@",  8,  "work tools"),
	Objective("##&&&", 8,  "social time"),
	Objective("###@@", 9,  "nice tools"),
	Objective("&&@@",  10, "public assistance"),
	Objective("&&&&&", 10, "direct community engagement in governance"),
	Objective("##%%",  10, "economic exchange"),
	Objective("##@@@", 11, "public resources"),
	Objective("###%%", 11, "commodoties exchange"),
	Objective("@@@@",  12, ""),
	Objective("&&%%",  12, "training"),
	Objective("&&&@@", 12, "infrastructure"),
	Objective("&&@@@", 13, "heavy infrastructure"),
	Objective("@@%%",  14, "free press"),
	Objective("&&&%%", 14, "group training"),
	Objective("##%%%", 14, ""),
	Objective("@@@@@", 15, "epic monument"),
	Objective("%%%%",  16, "technical education"),
	Objective("&&%%%", 16, "elite education"),
	Objective("@@@%%", 17, ""),
	Objective("@@%%%", 18, ""),
	Objective("%%%%%", 20, "high technology"),
	Objective("##&%",  9,  ""),
	Objective("&&@%",  12, ""),
	Objective("#@@%",  12, ""),
	Objective("##&&@@",13, ""),
	Objective("##&&%%",15, ""),
	Objective("##@@%%",17, ""),
	Objective("&&@@%%",19, ""),
	Objective("#&@%",  12, ""),
	Objective("###&@%",14, "public safety-net"),
	Objective("#&&&@%",16, "public works program"),
	Objective("#&@@@%",18, "public access to manufacturing"),
	Objective("#&@%%%",20, "public access to technology")
};
const PlayAction playstart[] = {
	PlayAction("",     "##",    "be resourceful"),
	PlayAction("..",   "++",    "prepare"),
	PlayAction("reset","cards", "reset actions")
};
const PlayAction play_deck[] = {
	PlayAction("###",  "%",     "experiment"),
	PlayAction("&",    "###",   "manage production"),
	PlayAction("",     "#&",    "negotiate"),
	PlayAction("",     "@",     "repurpose and reuse"),
	PlayAction("",	   "###",   "explore"),
	PlayAction("...",  "+++",   "get lucky"),
	PlayAction("@@",   "&&&##", "use heavy tools"),
	PlayAction("@@",   "%&##",  "use complicated tools"),
	PlayAction("%",    "@###",  "account for inventory"),
	PlayAction("&&",   "@###",  "guide workers"),
	PlayAction("&&&",  "@@##",  "show them how it's done"),
	PlayAction("%",    "&&##",  "coordinate"),
	PlayAction("####", "@@",    "craft something big"),
	PlayAction("",     "&##",   "inspire/intimidate/beg"),
	PlayAction("",     "####",  "inherit/acquire/steal"),
	PlayAction("",     "%",     "innovate"),
	PlayAction("",     "&&",    "leadership"),
	PlayAction("",     "#@",    "trade-favors/borrow"),
	PlayAction("##",   "@",     "craft"),
	PlayAction("&#",   "%",     "utilize expertise"),
	PlayAction("@",    "&&",    "use communication tools"),
	PlayAction("&&",   "%##",   "improve production process"),
	PlayAction("###",  "&@",    "create a production line"),
	PlayAction("@@",   "%&&",   "use smart tools"),
	PlayAction("&&&",  "%@#",   "empower others"),
	PlayAction("%",    "&&&",   "automate a critical task"),
	PlayAction("&&&",  "%%",    "educate and upskill"),
	PlayAction("%",    "@&#",   "make sure you're doing it right"),
	PlayAction("@",    "&&#",   "use domain-specialized tools"),
	PlayAction("@",    "&####", "use extraction machinery"),
	PlayAction("#####","%%",    "talent scout far and wide"),
	PlayAction("####", "@%",    "shop around"),
	PlayAction("%%",   "@@&&&", "automate production"),
	PlayAction("%%",   "@@@&#", "verify production quality"),
	PlayAction("#####","@@@",   "manufacture"),
	PlayAction("@##",  "%%",    "contract experts"),
	PlayAction("@@@",  "%%%",   "conference with experts"),
	PlayAction("&&&",  "@@@",   "plain-old hard work"),
	PlayAction("###",  "&&&",   "incentivise big"),
	PlayAction("##",   "&&",    "incentivise"),
	PlayAction("@@",   "%%",    "test the process"),
	PlayAction("&&",   "@@",    "iterate"),
	PlayAction("%",	   "@@",    "invent")
};

const ResourceType * g_resources = resources;
const Objective    * g_objective = objective;
const PlayAction   * g_play_deck = play_deck;
const PlayAction   * g_playstart = playstart;

const int g_len_resources = sizeof(resources)/sizeof(resources[0]);
const int g_len_objective = sizeof(objective)/sizeof(objective[0]);
const int g_len_play_deck = sizeof(play_deck)/sizeof(play_deck[0]);
const int g_len_playstart = sizeof(playstart)/sizeof(playstart[0]);
