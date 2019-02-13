#include <cstdio>
#include <cstdlib>
#include <termios.h>
#include <unistd.h>

#include "terminal.h"
#include "error.h"

using namespace terminal;

/*
	Terminal settings and I/O 
*/
struct termios orig_termios;

void terminal::exitRawMode() {
	if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1) error::die("tcsetatt");
}

void terminal::enterRawMode() {
	if(tcgetattr(STDIN_FILENO, &orig_termios) == -1) error::die("tcgetatt");
	atexit(terminal::exitRawMode);

	struct termios raw = orig_termios;
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	raw.c_oflag &= ~(OPOST);
	raw.c_cflag |= (CS8);
	raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 1;
	// passing back the attribute struct
	if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) error::die("tcsetatt");
}
