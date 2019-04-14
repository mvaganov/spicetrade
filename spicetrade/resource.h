#pragma once
#include <string>

class ResourceType {
public:
    std::string name;
    char icon;
    char color;
    ResourceType(std::string name, char icon, char color):name(name),icon(icon),color(color){}
};