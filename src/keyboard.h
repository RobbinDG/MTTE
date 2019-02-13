#pragma once

#include "cursor.h"
#include "render.h"

#define QUIT_TIMES 3

#define CTRL_KEY(k) ((k) & 0x1f)

enum KeyCodes {
	BACKSPACE = 127,
	ARROW_LEFT = 1000,
	ARROW_RIGHT,
	ARROW_UP,
	ARROW_DOWN,
	DEL_KEY,
	HOME_KEY,
	END_KEY,
	PAGE_UP,
	PAGE_DOWN
};

/*
	Represents a keyboard which input can be read and processed.
*/
class Keyboard {
private:
	Cursor& cursor;
	Render& render;
public:
	Keyboard(Cursor& cursor, Render& render);
	int readKey();
	void processKeypress();
};