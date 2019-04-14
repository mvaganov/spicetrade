#pragma once
#include <string>
#include "mem.h"
#include <stdio.h>

struct PlayAction {
    //const char * input, * output;
    std::string input, output;
    std::string name;//const char * name;
    PlayAction(const char * input, const char * output, const char * name)
        :input(input),output(output),name(name){}
    std::string ToString() const { return ToString(-1); }
    std::string ToString(int width) const {
        if(width < 0) {
            width = input.length()+1+output.length();
        }
        char* buffer = NEWMEM_ARR(char, width+1);
        int index = 0;
        while(index < width && input[index]){
            buffer[index] = input[index];
            index++;
        }
        if(index < width) {
            buffer[index++] = '>';
            int offset = index;
            while(index < width && output[index-offset]){
                buffer[index] = output[index-offset];
                index++;
            }
            while(index < width){
                buffer[index++] = ' ';
            }
        }
        buffer[index] = '\0';
        std::string result(buffer);
        DELMEM_ARR(buffer);
        return result;
    }
};
