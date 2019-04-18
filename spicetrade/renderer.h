#pragma once
#include "cards.h"
#include "graphics.h"

class Renderer
{
static void PrintAction (const PlayAction* a, int bg) {
	int ofcolor = CLI::getFcolor (), obcolor = CLI::getBcolor ();
	CLI::setColor (CLI::COLOR::LIGHT_GRAY, bg);

	const int width = 5;
	std::string str = a->input;
	int leadSpace = width - str.length ();
	for (int i = 0; i < leadSpace; ++i) { CLI::putchar (' '); }
	for (int i = 0; i < str.length (); ++i) {
		CLI::setColor (ColorOfRes (str[i]), bg);
		CLI::putchar (str[i]);
	}
	CLI::setColor (CLI::COLOR::LIGHT_GRAY, bg);
	CLI::putchar (':');
	str = a->output;
	leadSpace = width - str.length ();
	for (int i = 0; i < str.length (); ++i) {
		CLI::setColor (ColorOfRes (str[i]), bg);
		CLI::putchar (str[i]);
	}
	CLI::setColor (CLI::COLOR::LIGHT_GRAY, bg);
	for (int i = 0; i < leadSpace; ++i) { CLI::putchar (' '); }
	CLI::setColor (ofcolor, obcolor);
}
};;