#define NCURSES_NOMACROS
#include <ncursesw/ncurses.h>
#include <unistd.h>
#include <string>

int main() {
  initscr();
  WINDOW *wt = newwin(LINES - 3, COLS, 0, 0);
  WINDOW *wb = newwin(3, COLS, LINES - 3, 0);
  WINDOW *tw = derwin(wt, LINES - 3 - 2, COLS - 2, 1, 1);
  WINDOW *bw = derwin(wb, 3 - 2, LINES - 3 - 2, 1, 1);
  leaveok(wt,true);
  leaveok(tw,true);
  scrollok(tw, true);
  wsetscrreg(tw, 0, LINES - 3 - 2);
  box(wt, 0, 0);

  box(wb, 0, 0);
  wmove(wt, 1, 1);

  wprintw(tw,
          "test 11221 "
          "!\nfscas\ngsdgsd\ngdfd\nfgawer\ndhfdh\niytuy\newtet\nplpio\nnbvb");
  wnoutrefresh(wt);
  wnoutrefresh(wb);
  wnoutrefresh(tw);
  wnoutrefresh(bw);
  doupdate();
  usleep(3000000);
  bool i = true;
  while (1) {
    if (i) {
      // ===========================
      int y, x;
      getmaxyx(tw, y, x);
      wmove(wt, y, x);
      wclrtobot(wt);
      wresize(wt, LINES, COLS);
      wresize(tw, LINES, COLS - 2);
      // ===========================
      box(wt, 0, 0);
      
      wnoutrefresh(wt);
      wnoutrefresh(tw);
    } else {
      // ===========================
      wresize(wt, LINES - 3, COLS);
      touchwin(wb);
      // ===========================
      box(wt, 0, 0);

      wnoutrefresh(wt);
      wnoutrefresh(tw);
      wnoutrefresh(wb);
      wnoutrefresh(bw);
    }
    doupdate();
    i = !i;
    usleep(800000);
  }
}
