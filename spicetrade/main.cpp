#include <stdio.h>
#include "platform_conio.h"
#include "game.h"
#include "dictionary.h"
#include <string>
#include "cli.h"
#include "platform_random.h"

void print(std::string str) { CLI::printf("%s", str.c_str()); }
void print(const char * str) { CLI::printf("%s", str); }

Dictionary<char,const ResourceType*> resourceLookup;

char ColorOfRes(char icon) {
    const ResourceType * r = resourceLookup.Get(icon);
    return (r == NULL)?CLI::COLOR::LIGHT_GRAY:r->color;
}

void print(const PlayAction * a) {
    int ofcolor = CLI::getFcolor(), obcolor = CLI::getBcolor();
    int bg = CLI::COLOR::DARK_GRAY;
    CLI::setColor(CLI::COLOR::LIGHT_GRAY, bg);

    const int width = 5;
    std::string str = a->input;
    int leadSpace = width-str.length();
    for(int i = 0; i < leadSpace; ++i) { CLI::putchar(' '); }
    for(int i = 0; i < str.length(); ++i) {
        CLI::setColor(ColorOfRes(str[i]),bg);
        CLI::putchar(str[i]);
    }
    CLI::setColor(CLI::COLOR::LIGHT_GRAY, bg);
    CLI::putchar('>');
    str = a->output;
    leadSpace = width-str.length();
    for(int i = 0; i < str.length(); ++i) {
        CLI::setColor(ColorOfRes(str[i]),bg);
        CLI::putchar(str[i]);
    }
    CLI::setColor(CLI::COLOR::LIGHT_GRAY, bg);
    for(int i = 0; i < leadSpace; ++i) { CLI::putchar(' '); }
    CLI::setColor(ofcolor, obcolor);
}

int main(int argc, const char ** argv){
    for(int i = 0; i < argc; ++i) { printf("[%d] %s\n", i, argv[i]); }
 
    for(int i = 0; i < g_len_resources; ++i) {
        resourceLookup.Set(g_resources[i].icon, &(g_resources[i]));
    }

    Dictionary<std::string, const PlayAction*> actionDeck;
    VList<const PlayAction*> play_deck;
    for(int i = 0; i < g_len_play_deck; ++i) {
        actionDeck.Set(g_play_deck[i].input, &(g_play_deck[i]));
        play_deck.Add(&(g_play_deck[i]));
    }


    printf("%d possible actions\n", actionDeck.Count());
    //VList<Dictionary<std::string, const PlayAction*>::KVP> kvp(g_len_play_deck);
    //kvp.SetCount(g_len_play_deck);
    //actionDeck.ToArray(kvp.GetData());
    // for(int i = 0; i < g_len_playList; ++i) {
    //     const PlayAction * v = kvp[i].value;
    //     //printf("%s>%s : \"%s\"\n", v->input, v->output, v->name);
    //     printf("%s\n", v->ToString(10).c_str());
    // }

    platform_shuffle(play_deck.GetData(), 0, play_deck.Count());
    VList<const PlayAction*> hand;
    for(int i = 0; i < g_len_playstart; ++i) {
        hand.Add(&(g_playstart[i]));
    }
    for(int i = 0; i < 3; ++i) {
        hand.Add(play_deck.PopLast());
    }

    CLI::init();
    CLI::setSize(CLI::getWidth(), 25);
    CLI::fillScreen(' ');
    CLI::move(0,0);
    CLI::printf("Hello World!");
    int x = 10, y = 3;
    CLI::move(y, x);
    CLI::printf("Mr. V");
    int currentRow = 0;
    Dictionary<int,int> selectedMark;
    VList<int> selected;
    bool running = true;
    while(running) {
        // draw
        for(int i = 0; i < hand.Count(); ++i) {
            CLI::move(y+1+i, x);
            if(i == currentRow) {
                print(">");
            } else {
                print(" ");
            }
            bool isSelected = selectedMark.GetPtr(i) != NULL;
            if(isSelected) { print(" "); }
            print(hand[i]);
            if(isSelected) {
                int selectedIndex = selected.IndexOf(i);
                CLI::printf("%2d", selectedIndex);
            } else { print("   "); }
        } 
        // input
        int userInput = CLI::getch();
        CLI::printf("%d\n",userInput);
        // update
        switch(userInput) {
        case 'w':
            currentRow--;
            if(currentRow < 0){currentRow = 0;}
            break;
        case 's':
            currentRow++;
            if(currentRow >= hand.Count()){currentRow = hand.Count()-1;}
            break;
        case 'd': {
            int* isSelected = selectedMark.GetPtr(currentRow);
            if(isSelected == NULL) {
                selected.Add(currentRow);
                selectedMark.Set(currentRow, 1);
            }
        }
            break;
        case 'a': {
            int* isSelected = selectedMark.GetPtr(currentRow);
            if(isSelected != NULL) {
                int sindex = selected.IndexOf(currentRow);
                selected.RemoveAt(sindex);
                selectedMark.Remove(currentRow);
            }
        }
            break;
        case 'q':
            running = false;
            break;
        case '\n': case '\r':
            if(selected.Count() > 0 && currentRow >= 0 && currentRow < hand.Count()) {
                // remove the selected from the hand into a moving list, ordered by selected
                VList<const PlayAction*> reordered;
                for(int i = 0; i < selected.Count(); ++i) {
                    reordered.Add(hand[selected[i]]);
                }
                selected.Sort();
                for(int i = selected.Count()-1; i >= 0; --i) {
                    hand.RemoveAt(selected[i]);
                    if(selected[i] < currentRow) currentRow--;
                }
                // clear selected
                selected.Clear();
                selectedMark.Clear();
                // insert element into the list at the given index
                hand.Insert(currentRow, reordered.GetData(), reordered.Count());
                reordered.Clear();
            }
        }
    }
    CLI::release();

    return 0;
}