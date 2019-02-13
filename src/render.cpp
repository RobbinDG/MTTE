#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <iterator>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include "render.h"
#include "row.h"
#include "config.h"
#include "error.h"

int Render::getCursorPosition() {
	char buf[32];
	unsigned int i = 0;

  	if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

  	while (i < sizeof(buf) - 1) {
  	    if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
  	    if (buf[i] == 'R') break;
  	    i++;
  	}
  	buf[i] = '\0';

  	if (buf[0] != '\x1b' || buf[1] != '[') return -1;
  	if (std::sscanf(&buf[2], "%d;%d", &rows, &cols) != 2) return -1;
  	return 0;
}

int Render::getWindowSize() {
 	struct winsize ws;

  	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
  		if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
	    return getCursorPosition();
 	} else {
    	cols = ws.ws_col;
    	rows = ws.ws_row;
  	}
    return 0;
}

Render::Render() {
	if (getWindowSize() == -1) error::die("getWindowSize");
 	rows -= 2;
}

void Render::drawRows(std::string buf, std::vector<Row>& row) {
	for(int y = 0; y < rows; ++y) {
		int filerow = y + rowOffset;
		if(filerow >= 0 && (size_t)filerow >= row.size()) {
			if (row.size() == 0 && y == rows / 3) {
				std::ostringstream oss;
				oss << "MTTE -- version " << VERSION;
				std::string welcome = oss.str();
				if (cols >= 0 && welcome.length() > (size_t)cols) { 
					welcome.resize(cols);
					welcome.shrink_to_fit();
				}
				int padding = (cols - welcome.length()) / 2;
				if (padding) {
					buf += "~";
					padding--;
				}
				while (padding--) buf += " ";
			    buf += welcome.c_str();
			} else {
			    buf += "~";
			}
		} else {
			int len = row[filerow].getRSize() - colOffset;
			if(len < 0) len = 0;
			if (len > cols) len = cols;
			std::string r = row[filerow].getRendered();
			std::string::iterator c = r.begin() + colOffset;
			// char* c = &row[filerow].render[colOffset];
			std::string h = row[filerow].getHighlight();
			std::string::iterator hl = h.begin() + colOffset;
			// unsigned char* hl = &row[filerow].hl[colOffset];
			/*
			int curColour = -1;
			for(int j = 0; j < len; ++j) {
				if(iscntrl(c[j])) {
					char sym = c[j] <= 26 ? '@' + c[j] : '?';
					buf += "\x1b[7m";
					buf += &sym;
					buf += "\x1b[m";
					if(curColour != -1) {
						std::osstringstream oss;
						oss << "\x1b[" << curColour << "m";
						buf += oss.str();
					}
				}
				if(hl[j] == HL_NORMAL) {
					if(curColour != -1) {
						buf += "\x1b[39m";
						curColour = -1;
					}
					buf += &c[j];
				} else {
					int colour = editorSyntaxToColour(hl[j]);
					if(colour != curColour) {
						curColour = colour;
						std::osstringstream oss;
						oss << "\x1b[" << colour << "m";
						buf += oss.str();
					}
					buf += c[j];
				}
			}
			buf += "\x1b[39m";
			*/
		}

		buf += "\x1b[K";
		buf += "\r\n";
	}
}

void Render::refresh(std::vector<Row>& rows) {
	editorScroll();

 	std::string buf = "";
 	buf += "\x1b[?25l\x1b[1;1H";

	drawRows(buf, rows);
 	// editorDrawStatusBar(buf);
	// editorDrawMessageBar(buf);

	buf += "\x1b[H";

	fwrite(buf.c_str(), sizeof(char), buf.size(), stdout);

	// char buf[32];
 //  	snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - rowOffset) + 1, (E.rx - colOffset) + 1);
 //  	abAppend(&ab, buf, strlen(buf));

 //  	abAppend(&ab, "\x1b[?25h", 6);
 //  	write(STDOUT_FILENO, ab.b, ab.len);
 //  	abFree(&ab);
}


int Render::getRows() { return rows; }
int Render::getCols() { return cols; }
int Render::getrowOffsetset() { return rowOffset; }
int Render::getcolOffsetset() { return colOffset; }
int Render::getRx() { return rx; }

