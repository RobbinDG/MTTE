#pragma once

#include <vector>

#include "row.h"

class Render {
private:
	int rx;
	int rows, cols;
	int rowOffset, colOffset;
	int getCursorPosition();
	int getWindowSize();
public:
	Render();
	void drawRows(std::string buf, std::vector<Row>& row);
	void refresh(std::vector<Row>& row);
	int getRows();
	int getCols();
	int getRowOffset();
	int getColOffset();
	int getRx();
};