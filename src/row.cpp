#include <string>

#include "row.h"

Row::Row() {
	idx = 0;;
	chars = "";
	render = "";
	hl = NULL;
	bool hlOpenComment = false;
}

int Row::getSize() { return chars.size(); }
int Row::getRSize() { return render.size(); }
std::string Row::getRendered() { return render; }