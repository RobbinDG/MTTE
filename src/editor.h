#pragma once

#include <string>
#include <vector>
#include <ctime>

#include "cursor.h"
#include "render.h"
#include "keyboard.h"
#include "row.h"


class Editor {
private:
	Cursor cursor;
	Render render;
	std::vector<Row> row;
	int dirty;
	std::string filename;
	char statusmsg[80];
	time_t statusmsg_time;
	struct editorSyntax* syntax;
public:
	Editor();
	void processKeypress(int key);
	Keyboard assignKeyboard();
	void refreshScreen();
};