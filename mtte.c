/* includes */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

/* macros */

#define VERSION "0.1"

#define CTRL_KEY(k) ((k) & 0x1f)

enum editorKey {
	ARROW_LEFT = 1000,
	ARROW_RIGHT,
	ARROW_UP,
	ARROW_DOWN,
	HOME_KEY,
	END_KEY,
	PAGE_UP,
	PAGE_DOWN
};

/* data */

struct editorConfig {
	int cx, cy;
	int scrRows;
	int scrCols;
	struct termios orig_termios;	
};

struct editorConfig E;

/* terminal */

void die(const char *s) {
	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[1;1H", 6);
 	perror(s);
	exit(1);
}

void exitRawMode() {
	if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1) die("tcsetatt");
}

void enterRawMode() {
	if(tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetatt");
	atexit(exitRawMode);

	struct termios raw = E.orig_termios;
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	raw.c_oflag &= ~(OPOST);
	raw.c_cflag |= (CS8);
	raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 1;
	// passing back the attribute struct
	if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetatt");
}

/* Reads key input and translates escape sequences */
int editorReadKey() {
	int nread;
	char c;
	while((nread = read(STDIN_FILENO, &c, 1)) != 1) {
		if(nread == -1 && errno != EAGAIN) die("read");
	}

	if(c == '\x1b') {
		char seq[3];

		if(read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
		if(read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

		if(seq[0] == '[') {
			if(isdigit(seq[1])) {
				if(read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';				
				if(seq[2] == '~') {
					switch(seq[1]) {
						case '1': return HOME_KEY;
						case '4': return END_KEY;
						case '5': return PAGE_UP;
						case '6': return PAGE_DOWN;
						case '7': return HOME_KEY;
						case '8': return END_KEY;
					}
				}
			} else {
				switch(seq[1]) {
					case 'A': return ARROW_UP;
					case 'B': return ARROW_DOWN;
					case 'C': return ARROW_RIGHT;
					case 'D': return ARROW_LEFT;
					case 'H': return HOME_KEY;
					case 'F': return END_KEY;
				}
			}
		} else if(seq[0] = 'O') {
			switch(seq[1]) {
				case 'H': return HOME_KEY;
				case 'F': return END_KEY;
			}
		}
		return '\x1b';
	} else {
		return c;
	}
}

int getCursorPosition(int* rows, int* cols) {
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
  	if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;
  	return 0;
}

int getWindowSize(int* rows, int* cols) {
 	struct winsize ws;

  	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
  		if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
	    return getCursorPosition(rows, cols);
 	} else {
    	*cols = ws.ws_col;
    	*rows = ws.ws_row;
    	return 0;
  	}
}

/* append buffer */

#define ABUF_INIT {NULL, 0}

struct abuf {
	char* b;
	int len;
};

void abAppend(struct abuf *ab, const char *s, int len) {
	char *new = realloc(ab->b, ab->len + len);

	if (new == NULL) return;
	memcpy(&new[ab->len], s, len);
	ab->b = new;
	ab->len += len;
}

void abFree(struct abuf *ab) {
	free(ab->b);
}

/* input */

void editorMoveCursor(int key) {
	switch(key) {
		case ARROW_UP:
			if(E.cy != 0) E.cy--;
			break;
		case ARROW_LEFT:
			if(E.cy != 0) E.cx--;
			break;
		case ARROW_DOWN:
			if(E.cy != E.scrRows - 1) E.cy++;
			break;
		case ARROW_RIGHT:
			if(E.cx != E.scrCols - 1) E.cx++;
			break;
	}
}

void editorProcessKeypress() {
	int key = editorReadKey();

	switch(key) {
		case ARROW_UP:
		case ARROW_LEFT:
		case ARROW_DOWN:
		case ARROW_RIGHT:
			editorMoveCursor(key);
			break;

		case HOME_KEY:
			E.cx = 0;
			break;
		case END_KEY:
			

		case PAGE_UP:
		case PAGE_DOWN:
			{
				int n = E.scrRows;
				while(n--) {
					editorMoveCursor(key == PAGE_UP ? ARROW_UP : ARROW_DOWN);
				}
			}
			break;
		case CTRL_KEY('q'):
			write(STDOUT_FILENO, "\x1b[2J", 4);
			write(STDOUT_FILENO, "\x1b[1;1H", 6);
			exit(0);
			break;
	}
}

/* output */

void editorDrawRows(struct abuf* ab) {
	int rows = E.scrRows;
	for(int y = 0; y < rows; ++y) {
		if (y == E.scrRows / 3) {
		    char welcome[80];
		    int welcomelen = snprintf(welcome, sizeof(welcome),
		        "MTTE -- version %s", VERSION);
		if (welcomelen > E.scrCols) welcomelen = E.scrCols;
			int padding = (E.scrCols - welcomelen) / 2;
			if (padding) {
				abAppend(ab, "~", 1);
				padding--;
			}
			while (padding--) abAppend(ab, " ", 1);
		    abAppend(ab, welcome, welcomelen);
		} else {
		    abAppend(ab, "~", 1);
		}

		abAppend(ab, "\x1b[K", 3);
		if(y < rows-1) abAppend(ab, "\r\n", 2);
	}
}

void editorRefreshScreen() {
	struct abuf ab = ABUF_INIT;
	abAppend(&ab, "\x1b[?25l", 6);
	abAppend(&ab, "\x1b[1;1H", 6);

	editorDrawRows(&ab);

	abAppend(&ab, "\x1b[H", 3);

	char buf[32];
	snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.cy + 1, E.cx + 1);
	abAppend(&ab, buf, strlen(buf));

	abAppend(&ab, "\x1b[?25h", 6);
	write(STDOUT_FILENO, ab.b, ab.len);
	abFree(&ab);
}

/* init */

void initEditor() {
	E.cx = 0;
	E.cy = 0;
 	 if (getWindowSize(&E.scrRows, &E.scrCols) == -1) die("getWindowSize");
}

int main(int argc, char const *argv[]) {
	enterRawMode();
	initEditor();

	while(1) {
		editorRefreshScreen();
		editorProcessKeypress();
	}

	return 0;
}