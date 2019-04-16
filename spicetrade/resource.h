#pragma once
#include <string>

class ResourceType {
  public:
  	enum Type {none=0, resource=1, score=2};
	std::string name;
	char icon;
	char type;
	char color;
	ResourceType (std::string name, char icon, char type, char color)
	: name (name), icon (icon), type(type), color (color) {}
};