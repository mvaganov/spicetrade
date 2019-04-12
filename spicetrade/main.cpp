#include <stdio.h>
#include "platform_conio.h"
#include "game.h"

int main(int argc, const char ** argv){
    VList<const char*> l1;
    for(int i = 0; i < argc; ++i){
        l1.Add(argv[i]);
    }
    for(int i = 0; i < l1.Count(); ++i){
        printf("[%d] %s\n", i, l1[i]);
    }
    const TradeCard cards[] = {
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

    return 0;
}