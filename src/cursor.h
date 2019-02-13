#pragma once

/*
	Represents a cursor position on screen
*/
class Cursor {
private:
	int tabJump;
public:
	int x, y;
	Cursor(int tabJump = 1);
};