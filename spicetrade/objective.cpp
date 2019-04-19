#include "objective.h"
#include "game.h"
#include "graphics.h"

void Objective::PrintObjective (Game& g, const Objective* o, int bg) {
	int ofcolor = CLI::getFcolor (), obcolor = CLI::getBcolor ();
	CLI::setColor (CLI::COLOR::LIGHT_GRAY, bg);

	const int width = 8;
	std::string str = o?o->input:"";
	int leadSpace = (width - str.length ())/2;
	for (int i = 0; i < leadSpace; ++i) { CLI::putchar (' '); }
	for (int i = 0; i < str.length (); ++i) {
		CLI::setColor (g.ColorOfRes (str[i]), bg);
		CLI::putchar (str[i]);
	}
	CLI::setColor (CLI::COLOR::LIGHT_GRAY, bg);
	// CLI::putchar (':');
	// str = a->output;
	leadSpace = width - (leadSpace + str.length ());
	// for (int i = 0; i < str.length (); ++i) {
	// 	CLI::setColor (ColorOfRes (str[i]), bg);
	// 	CLI::putchar (str[i]);
	// }
	CLI::setColor (CLI::COLOR::LIGHT_GRAY, bg);
	for (int i = 0; i < leadSpace; ++i) { CLI::putchar (' '); }
	CLI::setColor (ofcolor, obcolor);
}
