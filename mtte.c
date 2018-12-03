/* includes */

#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/types.h>

/* macros */

#define VERSION "0.1"
#define TAB_SIZE 2
#define QUIT_TIMES 3

#define CTRL_KEY(k) ((k) & 0x1f)

enum editorKey {
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

/* data */

typedef struct erow {
	int size;
	int rsize;
	char* chars;
	char* render;
} erow;

struct editorConfig {
	int cx, cy;
	int rx;
	int scrRows;
	int scrCols;
	int numRows;
	int rowOff;
	int colOff;
	erow* row;
	int dirty;
	char* filename;
	char statusmsg[80];
	time_t statusmsg_time;
	struct termios orig_termios;	
};

struct editorConfig E;

/* Prototypes */

void editorSetStatusMessage(const char *fmt, ...);

/* Tcerminal */

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
						case '3': return DEL_KEY;
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
					case 'P': return DEL_KEY;
				}
			}
		} else if(seq[0] == 'O') {
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

/* Row operations */

int editorCxToRx(erow* row, int cx) {
	int rx = 0;
	for(int i = 0; i < cx; ++i) {
		rx += (row->chars[i] == '\t') ? TAB_SIZE - rx % TAB_SIZE : 1;
	}
	return rx;
}

void editorUpdateRow(erow* row) {
	int tabs = 0;
	for(int j = 0; j < row->size; ++j) {
		if(row->chars[j] == '\t') ++tabs;
	}


	free(row->render);
	row->render = malloc(row->size + tabs*(TAB_SIZE-1) + 1);

	int idx = 0;
	for(int j = 0; j < row->size; ++j) {
		if(row->chars[j] == '\t') {
			do {
				row->render[idx++] = ' ';
			} while(idx % TAB_SIZE != 0);
		} else
			row->render[idx++] = row->chars[j];
	}
	row->render[idx] = '\0';
	row->rsize = idx;
}

void editorInsertRow(int at, char *s, size_t len) {
	if(at < 0 || at > E.numRows) return;

	E.row = realloc(E.row, sizeof(erow) * (E.numRows + 1));
	memmove(&E.row[at+1], &E.row[at], sizeof(erow) * (E.numRows - at));

	E.row[at].size = len;
	E.row[at].chars = malloc(len + 1);
	memcpy(E.row[at].chars, s, len);
	E.row[at].chars[len] = '\0';
	E.numRows++;
	E.dirty++;

	E.row[at].rsize = 0;
	E.row[at].render = NULL;
	editorUpdateRow(&E.row[at]);
}

void editorFreeRow(erow* row) {
	free(row->render);
	free(row->chars);
}

void editorDeleteRow(int at) {
	if(at < 0 || at > E.numRows) return;
	editorFreeRow(&E.row[at]);
	memmove(&E.row[at], &E.row[at+1], sizeof(erow) * (E.numRows - at - 1));
	E.numRows--;
	E.dirty++;
}

void editorRowInsertChar(erow *row, int at, char c) {
	if(at < 0 || at > row->size) at = row->size;
	// extra char + null byte
	row->chars = realloc(row->chars, row->size + 2);
	memmove(&row->chars[at+1], &row->chars[at], row->size - at + 1);
	row->size++;
	row->chars[at] = c;
	editorUpdateRow(row);
	E.dirty++;
}

void editorInsertNewLine() {
	if(E.cx == 0) {
		editorInsertRow(E.cy, "", 0);
	} else {
		erow* row = &E.row[E.cy];
		editorInsertRow(E.cy+1, &row->chars[E.cx], row->size - E.cx);
		row = &E.row[E.cy];
		row->size = E.cx;
		row->chars[row->size] = '\0';
		editorUpdateRow(row);
	}
	E.cy++;
	E.cx = 0;
}

void editorRowAppendString(erow* row, char* s, size_t len) {
	row->chars = realloc(row->chars, row->size + len + 1);
	memcpy(&row->chars[row->size], s, len);
	row->size += len;
	row->chars[row->size] = '\0';
	editorUpdateRow(row);
	E.dirty++;
}

void editorRowDelChar(erow* row, int at) {
	if(at < 0 || at > row->size) return;

	memmove(&row->chars[at], &row->chars[at+1], row->size - at);
	row->size--;
	editorUpdateRow(row);
	E.dirty++;
}

/* Row Operations */

void editorInsertChar(char c) {
	// Cursor at new row
	if(E.cy == E.numRows) editorInsertRow(E.numRows, "", 0);
	editorRowInsertChar(&E.row[E.cy], E.cx, c);
	E.cx++;
}

void editorDeleteChar() {
	if(E.cy == E.numRows) return;
	if(E.cx == 0 && E.cy == 0) return;

	erow* row = &E.row[E.cy];
	if(E.cx > 0) {
		editorRowDelChar(row, (E.cx--) - 1);
	} else {
		E.cx = E.row[E.cy-1].size;
		editorRowAppendString(&E.row[E.cy-1], row->chars, row->size);
		editorDeleteRow(E.cy--);
	}
}

/* File I/O */

char* editorRowsToString(int* buflen) {
	int totlen = 0;
	for(int j = 0; j < E.numRows; ++j)
		totlen += E.row[j].size + 1;
	*buflen = totlen;

	char *buf = malloc(totlen * sizeof(char));
	char* p = buf;
	for(int j = 0; j < E.numRows; ++j) {
		memcpy(p, E.row[j].chars, E.row[j].size);
		p += E.row[j].size;
		*(p++) = '\n';
	}
	return buf;
}

void editorOpen(char* filename) {
	free(E.filename);
	E.filename = strdup(filename);

	FILE *fp = fopen(filename, "r");
	if(!fp) die("fopen");

	char* line = NULL;
	size_t linecap = 0;
	ssize_t linelen;

	while((linelen = getline(&line, &linecap, fp)) != -1) {
		while(linelen > 0 && (line[linelen-1] == '\n' || line[linelen-1] == '\r')) linelen--;
		editorInsertRow(E.numRows, line, linelen);
	}

	free(line);
	fclose(fp);
	E.dirty = 0;
}

void editorSave() {
	if(E.filename == NULL) return;

	int len;
	char* buf = editorRowsToString(&len);

	// Allow Read/Write and Creating. Default permission apply (0644)
	int fd = open(E.filename, O_RDWR | O_CREAT, 0644);
	if(fd != -1) {
		 if(ftruncate(fd, len) != -1) {
			if(write(fd, buf, len) == len) {
				close(fd);
				free(buf);
				editorSetStatusMessage("%d bytes saved to %s", len, E.filename);
				E.dirty = 0;
				return;
			}
		}
		close(fd);
	}
	free(buf);
	editorSetStatusMessage("Couldn't save; I/O error: %s", strerror(errno));
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
	erow* row = (E.cy >= E.numRows) ? NULL : &E.row[E.cy];

	switch(key) {
		case ARROW_UP:
			if(E.cy != 0) E.cy--;
			break;
		case ARROW_LEFT:
			if(E.cx != 0) E.cx--;
			else if(E.cy > 0) {
				E.cy--;
				E.cx = E.row[E.cy].size;
			}
			break;
		case ARROW_DOWN:
			if(E.cy < E.numRows) E.cy++;
			break;
		case ARROW_RIGHT:
			if(row && E.cx < row->size) E.cx++;
			else if(row && E.cy < E.numRows) {
				E.cy++;
				E.cx = 0;
			}
			break;
	}

	row = (E.cy >= E.numRows) ? NULL : &E.row[E.cy];
	int rowLen = row ? row->size : 0;
	if(row && E.cx > rowLen) E.cx = rowLen;
}

void editorProcessKeypress() {
	static int quitTimes = QUIT_TIMES;
	int key = editorReadKey();

	switch(key) {
		case '\r':
			editorInsertNewLine();
			break;

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
			if(E.cy < E.numRows)
				E.cx = E.row[E.cy].size;
			break;

		case BACKSPACE:
		case CTRL_KEY('h'):
		case DEL_KEY:
			if(key == DEL_KEY) editorMoveCursor(ARROW_RIGHT);
			editorDeleteChar();
			break;

		case PAGE_UP:
		case PAGE_DOWN:
			{
				if(key == PAGE_UP)
					E.cy = E.rowOff;
				else if(key == PAGE_DOWN)
					E.cy = E.rowOff + E.scrRows - 1;

				int n = E.scrRows;
				while(n--) {
					editorMoveCursor(key == PAGE_UP ? ARROW_UP : ARROW_DOWN);
				}
			}
			break;

		case CTRL_KEY('l'):
		case '\x1b':
			break;

		case CTRL_KEY('s'):
			editorSave();
			break;

		case CTRL_KEY('q'):
			if(E.dirty && quitTimes > 0) {
				editorSetStatusMessage("There are unsaved changes. Press CTRL+Q %d more times to quit.", quitTimes--);
				return;
			}
			write(STDOUT_FILENO, "\x1b[2J", 4);
			write(STDOUT_FILENO, "\x1b[1;1H", 6);
			exit(0);
			break;

		default:
			editorInsertChar((char)key);
			break;
	}
	quitTimes = QUIT_TIMES;
}

/* output */

void editorScroll() {
	E.rx = 0;
	if(E.cy < E.numRows) E.rx = editorCxToRx(&E.row[E.cy], E.cx);

	if(E.cy < E.rowOff) E.rowOff = E.cy;
	if(E.cy >= E.rowOff + E.scrRows) E.rowOff = E.cy - E.scrRows + 1;
	if(E.rx < E.colOff) E.colOff = E.rx;
	if(E.rx >= E.colOff + E.scrCols) E.colOff = E.rx - E.scrCols + 1;
}

void editorDrawRows(struct abuf* ab) {
	int rows = E.scrRows;
	for(int y = 0; y < rows; ++y) {
		int filerow = y + E.rowOff;
		if(filerow >= E.numRows) {
			if (E.numRows == 0 && y == E.scrRows / 3) {
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
		} else {
			int len = E.row[filerow].rsize - E.colOff;
			if(len < 0) len = 0;
			if (len > E.scrCols) len = E.scrCols;
			abAppend(ab, &E.row[filerow].render[E.colOff], len);
		}

		abAppend(ab, "\x1b[K", 3);
		abAppend(ab, "\x1b[K", 3);
		abAppend(ab, "\r\n", 2);
	}
}

void editorDrawStatusBar(struct abuf *ab) {
	abAppend(ab, "\x1b[7m", 4);

	char lstatus[80], rstatus[80];
	int llen = snprintf(lstatus, sizeof(lstatus), "> %.20s%s",
		E.filename ? E.filename : "[No Name]",
		E.dirty ? " (Modified)" : "");
	int rlen = snprintf(rstatus, sizeof(rstatus), "[%d/%d]", E.cy+1, E.numRows);
	if(llen > E.scrCols) llen = E.scrCols;
	abAppend(ab, lstatus, llen);

	while(llen < E.scrCols) {
		if(E.scrCols - llen == rlen) {
			abAppend(ab, rstatus, rlen);
			break;
		} else {
			abAppend(ab, " ", 1);
			llen++;
		}
	}
	abAppend(ab, "\x1b[m", 3);
	abAppend(ab, "\r\n", 2);
}

void editorDrawMessageBar(struct abuf* ab) {
	abAppend(ab, "\x1b[K", 3);
	int msgLen = strlen(E.statusmsg);
	if(msgLen > E.scrCols) msgLen = E.scrCols;
	if(msgLen && time(NULL) - E.statusmsg_time < 5) {
		abAppend(ab, E.statusmsg, msgLen);
	}
}

void editorRefreshScreen() {
	editorScroll();

	struct abuf ab = ABUF_INIT;
	abAppend(&ab, "\x1b[?25l", 6);
	abAppend(&ab, "\x1b[1;1H", 6);

	editorDrawRows(&ab);
	editorDrawStatusBar(&ab);
	editorDrawMessageBar(&ab);

	abAppend(&ab, "\x1b[H", 3);

	char buf[32];
	snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.rowOff) + 1, (E.rx - E.colOff) + 1);
	abAppend(&ab, buf, strlen(buf));

	abAppend(&ab, "\x1b[?25h", 6);
	write(STDOUT_FILENO, ab.b, ab.len);
	abFree(&ab);
}

void editorSetStatusMessage(const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(E.statusmsg, sizeof(E.statusmsg), fmt, ap);
	va_end(ap);
	E.statusmsg_time = time(NULL);
}

/* init */

void initEditor() {
	E.cx = 0;
	E.cy = 0;
	E.numRows = 0;
	E.rowOff = 0;
	E.colOff = 0;
	E.row = NULL;
	E.dirty = 0;
	E.filename = NULL;
	E.statusmsg[0] = '\0';
	E.statusmsg_time = 0;

 	if (getWindowSize(&E.scrRows, &E.scrCols) == -1) die("getWindowSize");
 	E.scrRows -= 2;
}

int main(int argc, char *argv[]) {
	enterRawMode();
	initEditor();
	if(argc >= 2) {
		editorOpen(argv[1]);
	}

	editorSetStatusMessage("HELP: Ctrl + Q = Quit | Ctrl + S = Save");

	while(1) {
		editorRefreshScreen();
		editorProcessKeypress();
	}

	return 0;
}