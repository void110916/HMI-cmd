#define NCURSES_NOMACROS
#include <ncursesw/ncurses.h>
#include <unistd.h>

#include <string>
WINDOW *twin, *bwin;
#define SCORE_SIZE 3
#define ISCTRL(ch) ((ch) & 0x1f)
using namespace std;
int main() {
  // init global window
  initscr();
  // ======================
  //   set keyboard property
  keypad(stdscr, TRUE);
  // cbreak();
  noecho();
  // nonl();  // no new line when press 'enter'

  curs_set(1);
  // cbreak();
  raw();  // disable signal (ex. ctrl+c)

  nodelay(stdscr, true);

  refresh();
  // intrflush(stdscr, true);
  // ======================

  twin = newwin(LINES - SCORE_SIZE, COLS, 0, 0);
  bwin = newwin(SCORE_SIZE, COLS, LINES - SCORE_SIZE, 0);

  box(twin, 0, 0);
  wmove(twin, 1, 1);

  box(bwin, 0, 0);
  wmove(bwin, 1, 1);

  // keypad(bwin, TRUE);
  wrefresh(twin);
  wrefresh(bwin);
  string lstr;
  while (1) {
    int ch = getch();  // use getch will reset cursor to (1,1)
    int s = '\r';
    if (ch != ERR) {
      switch (ch) {
        case ISCTRL('o'):  // ESC
        {
          endwin();
          return 0;
          break;
        }
        case '\n':  // enter, KEY_ENTER not working
        {
          // lstr.push_back('\n');
          wprintw(twin, "%s\n", lstr.c_str());
          box(twin, 0, 0);
          wrefresh(twin);
          wclear(bwin);
          box(bwin, 0, 0);
          wmove(bwin, 1, 1);
          wrefresh(bwin);
          lstr.clear();
          break;
        }
        case KEY_BACKSPACE:  // backspace
        {
          int x, y;
          getyx(bwin, y, x);
          if (x < 2) break;

          //   wdelch(bwin);
          //   mvwaddch(bwin,y,x-2,' ');
          const char space = ' ';
          mvwaddnstr(bwin, y, x - 1, &space, 1);
          wmove(bwin, y, x - 1);
          wrefresh(bwin);
          lstr.pop_back();
          break;
        }
        case KEY_DL: {
          int x, y;
          getyx(bwin, y, x);
          const char space = ' ';
          mvwaddnstr(bwin, y, x, &space, 1);
          wrefresh(bwin);
          lstr.pop_back();
          break;
        }
        case KEY_LEFT: {
          int x, y;
          getyx(bwin, y, x);
          if (x == 1) {
            if (y > 1) {
              wmove(bwin, y - 1, x);
              wrefresh(bwin);
            }
            break;
          }
          wmove(bwin, y, x - 1);
          wrefresh(bwin);
          break;
        }
        case KEY_RIGHT: {
          int h = 0;
          break;
        }
        case KEY_DOWN:
          //   focus->keyDown();
          //   focus->wrefresh();
          {
            int h = 0;
            break;
          }
        case KEY_UP:
          //   focus->keyUp();
          //   focus->wrefresh();
          {
            int h = 0;
            break;
          }
          case KEY_DC:
          //   focus->keyUp();
          //   focus->wrefresh();
          {
            int h = 0;
            break;
          }
          case KEY_F(1):
          {
            
          }
        default:
          //   waddch();
          char c = ch & 0xff;
          waddnstr(bwin, &c, 1);
          // //   wprintw(bwin, &ch);
          wrefresh(bwin);
          lstr.push_back(ch);
          //   str[pCur++] = ch;

          break;
      }
    }
    // wrefresh(twin);
    // wrefresh(bwin);
    // refresh(); // this will reset cursor to (1,1)
    usleep(100);
  }
  endwin();
  return 0;
}