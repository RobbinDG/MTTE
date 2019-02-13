#pragma once

class Row {
private:
	int idx;
	std::string chars;
	std::string render;
	unsigned char* hl;
	bool hlOpenComment;
public:
	Row();
	int getSize();
	int getRSize();
};