#include <stdio.h>
#include "platform_conio.h"
#include "game.h"
#include "rbtree.h"
#include <string>

int main(int argc, const char ** argv){
    VList<const char*> l1;
    std::string a = "abc", b = "def";
    for(int i = 0; i < argc; ++i){
        l1.Add(argv[i]);
    }
    for(int i = 0; i < l1.Count(); ++i){
        printf("[%d] %s\n", i, l1[i]);
    }
    const Objective objective[] = {
        Objective("YYRR",	6),
        Objective("YYYRR",	7),
        Objective("RRRR",	8),
        Objective("YYGG",	8),
        Objective("YYRRR",	8),
        Objective("YYYGG",	9),
        Objective("RRGG",	10),
        Objective("RRRRR",	10),
        Objective("YYBB",	10),
        Objective("YYGGG",	11),
        Objective("YYYBB",	11),
        Objective("GGGG",	12),
        Objective("RRBB",	12),
        Objective("RRRGG",	12),
        Objective("RRGGG",	13),
        Objective("GGBB",	14),
        Objective("RRRBB",	14),
        Objective("YYBBB",	14),
        Objective("GGGGG",	15),
        Objective("BBBB",	16),
        Objective("RRBBB",	16),
        Objective("GGGBB",	17),
        Objective("GGBBB",	18),
        Objective("BBBBB",	20),
        Objective("YYRB",	9),
        Objective("RRGB",	12),
        Objective("YGGB",	12),
        Objective("YYRRGG",	13),
        Objective("YYRRBB",	15),
        Objective("YYGGBB",	17),
        Objective("RRGGBB",	19),
        Objective("YRGB",	12),
        Objective("YYYRGB",	14),
        Objective("YRRRGB",	16),
        Objective("YRGGGB",	18),
        Objective("YRGBBB",	20)
    };
    const TradeCard tradeList[] = {
        TradeCard("", "YY"),
        TradeCard("",	"YY"),
        TradeCard("..", "++"),
        TradeCard("YYY",	"B"),
        TradeCard("R",	"YYY"),
        TradeCard("",	"YR"),
        TradeCard("",	"G"),
        TradeCard("",	"YYY"),
        TradeCard("...", "+++"),
        TradeCard("GG",	"RRRYY"),
        TradeCard("GG",	"BRYY"),
        TradeCard("B",	"GYYY"),
        TradeCard("RR",	"GYYY"),
        TradeCard("RRR",	"GGYY"),
        TradeCard("B",	"RRYY"),
        TradeCard("YYYY",	"GG"),
        TradeCard("",	"RYY"),
        TradeCard("",	"YYYY"),
        TradeCard("",	"B"),
        TradeCard("",	"RR"),
        TradeCard("",	"YG"),
        TradeCard("YY",	"G"),
        TradeCard("RY",	"B"),
        TradeCard("G",	"RR"),
        TradeCard("RR",	"BYY"),
        TradeCard("YYY",	"RG"),
        TradeCard("GG",	"BRR"),
        TradeCard("RRR",	"BGY"),
        TradeCard("B",	"RRR"),
        TradeCard("RRR",	"BB"),
        TradeCard("B",	"GRY"),
        TradeCard("G",	"RRY"),
        TradeCard("G",	"RYYYY"),
        TradeCard("YYYYY",	"BB"),
        TradeCard("YYYY",	"GB"),
        TradeCard("BB",	"GGRRR"),
        TradeCard("BB",	"GGGRY"),
        TradeCard("YYYYY",	"GGG"),
        TradeCard("GYY",	"BB"),
        TradeCard("GGG",	"BBB"),
        TradeCard("RRR",	"GGG"),
        TradeCard("YYY",	"RRR"),
        TradeCard("YY",	"RR"),
        TradeCard("GG",	"BB"),
        TradeCard("RR",	"GG"),
        TradeCard("B",	"GG"),
    };
    int len_tradeList = sizeof(tradeList)/sizeof(tradeList[0]);
    RBTree<std::string, std::string> trades;
    for(int i = 0; i < len_tradeList; ++i) {
        trades.Set(tradeList[i].from, tradeList[i].to);
    }
    // printf("%d possible trades\n", trades.Count());
    // RBTree<std::string, std::string>::KVP * kvp = new RBTree<std::string, std::string>::KVP[len_tradeList];
    // for(int i = 0; i < len_tradeList; ++i) {
    //     printf("%s -> %s\n", kvp[i].key.c_str(), kvp[i].value.c_str());
    // }
    // delete [] kvp;
    return 0;
}