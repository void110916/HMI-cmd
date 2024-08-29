#define NCURSES_NOMACROS
#include <ncursesw/ncurses.h>
#include <unistd.h>
#include <string>

int main() {
  initscr();
  WINDOW *w = newwin(LINES-3, COLS, 0, 0);
  WINDOW *wl = newwin(3, COLS, LINES-3, 0);
  box(w, 0, 0);
  
  box(wl, 0, 0);
  wmove(w,1,1);

  wprintw(w,"test 11221 !\nfscas");
  wrefresh(w);
  wrefresh(wl);
  usleep(3000000);
  bool i = true;
  while (1) {
    if (i) {
      // way 1 =====================
      // wclear(wl);
      // wresize(w, LINES, COLS);
      // wclear(w);
      // ===========================
      // way 2 =====================
      wclrtobot(w);
      wresize(w, LINES, COLS);
      // ===========================
      box(w, 0, 0);
      touchwin(w);
      wnoutrefresh(w);
    } else {
      // way 1 =====================
      /// wclear(wl);   // clear old things from background
      /// wresize(w, LINES-3, COLS);
      /// wclear(w);
      // ===========================
      // way 2 =====================
      wresize(w, LINES-3, COLS);
      touchwin(w);
      touchwin(wl);
      // ===========================
      box(w, 0, 0);
      box(wl, 0, 0);

    //   wrefresh(stdscr);
      wnoutrefresh(w);
      wnoutrefresh(wl);
    }
    // touchwin(wl);
    // wnoutrefresh(wl);
    doupdate();
    i = !i;
    usleep(800000);
  }
}

// void draw_borders(WINDOW *screen) {
//   int x, y, i;

//   getmaxyx(screen, y, x);

//   // 4 corners
//   mvwprintw(screen, 0, 0, "+");
//   mvwprintw(screen, y - 1, 0, "+");
//   mvwprintw(screen, 0, x - 1, "+");
//   mvwprintw(screen, y - 1, x - 1, "+");

//   // sides
//   for (i = 1; i < (y - 1); i++) {
//     mvwprintw(screen, i, 0, "|");
//     mvwprintw(screen, i, x - 1, "|");
//   }

//   // top and bottom
//   for (i = 1; i < (x - 1); i++) {
//     mvwprintw(screen, 0, i, "-");
//     mvwprintw(screen, y - 1, i, "-");
//   }
// }

// int main(int argc, char *argv[]) {
//   int parent_x, parent_y, new_x, new_y;
//   int score_size = 3;

//   initscr();
//   noecho();
//   curs_set(FALSE);

//   // set up initial windows
//   getmaxyx(stdscr, parent_y, parent_x);

//   WINDOW *field = newwin(parent_y - score_size, parent_x, 0, 0);
//   WINDOW *score = newwin(score_size, parent_x, parent_y - score_size, 0);

//   draw_borders(field);
//   draw_borders(score);

//   while(1) {
//     getmaxyx(stdscr, new_y, new_x);

//     if (new_y != parent_y || new_x != parent_x) {
//       parent_x = new_x;
//       parent_y = new_y;

//       wresize(field, new_y - score_size, new_x);
//       wresize(score, score_size, new_x);
//       mvwin(score, new_y - score_size, 0);

//       wclear(stdscr);
//       wclear(field);
//       wclear(score);

//       draw_borders(field);
//       draw_borders(score);
//     }

//     // draw to our windows
//     mvwprintw(field, 1, 1, "Field");
//     mvwprintw(score, 1, 1, "Score");

//     // refresh each window
//     wrefresh(field);
//     wrefresh(score);
//   }

//   endwin();

//   return 0;
// }