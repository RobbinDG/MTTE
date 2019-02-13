#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <ctime>

#include "editor.h"
#include "cursor.h"
#include "render.h"
#include "keyboard.h"

Editor::Editor() {
  Cursor cursor = Cursor(1);
  Render render = Render();
	row = {};
	dirty = 0;
	filename = "";
	statusmsg[0] = '\0';
	statusmsg_time = 0;
	syntax = NULL;
}


void Editor::processKeypress(int key) {
  static int quitTimes = QUIT_TIMES;

  switch(key) {
    case '\r':
      // editorInsertNewLine();
      break;

    case ARROW_UP:
    case ARROW_LEFT:
    case ARROW_DOWN:
    case ARROW_RIGHT:
      // editorMoveCursor(key);
      break;

    case HOME_KEY:
      cursor.x = 0;
      break;
    case END_KEY:
      if(cursor.y >= 0 && (size_t) cursor.y < row.size())
        cursor.x = row[cursor.y].getSize();
      break;

    case BACKSPACE:
    case CTRL_KEY('h'):
    case DEL_KEY:
      // if(key == DEL_KEY) editorMoveCursor(ARROW_RIGHT);
      // editorDeleteChar();
      break;

    case PAGE_UP:
    case PAGE_DOWN:
      {
        if(key == PAGE_UP)
          cursor.y = render.getRowOffset();
        else if(key == PAGE_DOWN)
          cursor.y = render.getRowOffset() + render.getRows() - 1;

        int n = render.getRows();
        while(n--) {
          // editorMoveCursor(key == PAGE_UP ? ARROW_UP : ARROW_DOWN);
        }
      }
      break;

    case CTRL_KEY('l'):
    case '\x1b':
      break;

    case CTRL_KEY('s'):
      // editorSave();
      break;

    case CTRL_KEY('f'):
      // editorFind();
      break;

    case CTRL_KEY('q'):
      if(dirty && quitTimes > 0) {
        // editorSetStatusMessage("There are unsaved changes. Press CTRL+Q %d more times to quit.", quitTimes--);
        return;
      }
      fwrite("\x1b[2J", sizeof(char), 4, stdout);
      fwrite("\x1b[1;1H", sizeof(char), 6, stdout);
      // write(STDOUT_FILENO, "\x1b[2J", 4);
      // write(STDOUT_FILENO, "\x1b[1;1H", 6);
      exit(0);
      break;

    default:
      // editorInsertChar((char)key);
      break;
  }
  quitTimes = QUIT_TIMES;
}

Keyboard Editor::assignKeyboard() {
    return Keyboard(cursor, render);
}

void Editor::refreshScreen() {
    render.refresh();
}