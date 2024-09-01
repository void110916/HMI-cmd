#define NCURSES_NOMACROS
#include <ncursesw/ncurses.h>
#include <unistd.h>

#include <boost/algorithm/string.hpp>
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

  nodelay(stdscr, true);  // stdscr use with getch, win use with wgetch

  refresh();
  // intrflush(stdscr, true);
  // ======================
  WINDOW *tb = newwin(LINES - SCORE_SIZE, COLS, 0, 0);
  WINDOW *bb = newwin(SCORE_SIZE, COLS, LINES - SCORE_SIZE, 0);
  box(tb, 0, 0);
  box(bb, 0, 0);
  mvwprintw(tb,0,1," term ");
  wrefresh(tb);
  wrefresh(bb);
  // touchwin(stdscr);
  // refresh();
  twin = derwin(tb, LINES - SCORE_SIZE - 2, COLS - 2, 1, 1);
  bwin = derwin(tb, SCORE_SIZE-2, COLS - 2,  1, 1);
  
  
  scrollok(twin, true);
  wsetscrreg(twin, 2, LINES - SCORE_SIZE - 3);
  wmove(bwin, 0, 0);

  // keypad(bwin, TRUE);
  wrefresh(twin);
  wrefresh(bwin);

  string lstr;
  bool isshort = true;
  for (int i = 0; i < 300; i++) {
    wprintw(twin, "%d..............................................",i);
  }

  wrefresh(twin);
  while (1) {
    int ch = wgetch(bwin);  // use getch will reset cursor to (1,1)
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
          string str(COLS - 2, 0);
          mvwinnstr(bwin, 1, 1, str.data(), COLS - 2);
          boost::trim_right(str);

          wprintw(twin, "%s\n", str.c_str());
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
        case KEY_DOWN: {
          int h = 0;
          break;
        }
        case KEY_UP: {
          int h = 0;
          break;
        }
        case KEY_DC: {
          int h = 0;
          break;
        }
        case ISCTRL('c'): {
          if (isshort) {
            wresize(tb, LINES - SCORE_SIZE - 2, COLS);
            wclear(tb);
            // wresize(twin, LINES - SCORE_SIZE - 2 - 2, COLS - 2);
          } else {
            wresize(tb, LINES - SCORE_SIZE, COLS);
            // wresize(twin, LINES - SCORE_SIZE - 2, COLS - 2);
          }
          // touchwin(stdscr);
          wrefresh(tb);
          refresh();
          // wrefresh(twin);
          isshort != isshort;
          break;
        }
        default:
          //   waddch();
          char c = ch & 0xff;
          waddch(bwin, c);
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