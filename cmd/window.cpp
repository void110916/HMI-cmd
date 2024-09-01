#include "window.h"

std::vector<Window *> Window::windows;

Window::~Window() {}

Window::Window(int height, int width, int startY, int startX, std::string name,
               bool leavok)
    : height(height), width(width), startY(startY), startX(startX) {
  wbox = newwin(height, width, startY, startX);
  box(wbox, 0, 0);

  w = derwin(wbox, height - 2, width - 2, 1, 1);
  nodelay(w,true);
  leaveok(w, leavok);
  leaveok(wbox, leavok);
  if (!name.empty()) {
    this->name = name;
    mvwprintw(wbox, 0, 1, name.c_str());
    scrollok(w, true);
    wsetscrreg(w, 0, height - 2);
  }

  windows.push_back(this);
}

int Window::wrefresh() { return ::wrefresh(w); }

int Window::refresh() {
  // touchwin(stdscr);
  return ::refresh();
}

void Window::waitUpdate() {
  wnoutrefresh(wbox);
  wnoutrefresh(w);
}
void Window::update() { doupdate(); }
void Window::updateAll() {
  for (auto win : windows) win->waitUpdate();
  doupdate();
}

void Window::resize(int height, int width) {
  this->height = height;
  this->width = width;
  int y, x;
  getmaxyx(w, y, x);
  wmove(wbox, y, x);
  wclrtobot(wbox);
  wresize(w, height - 2, width - 2);
  wresize(wbox, height, width);

  box(wbox, 0, 0);

  if (!name.empty()) {
    mvwprintw(wbox, 0, 1, name.c_str());
    wsetscrreg(w, 0, height - 2);
  }
}

void Window::touch() {
  touchwin(wbox);
  touchwin(w);
}

void Window::untouch() {
  untouchwin(wbox);
  untouchwin(w);
}

void Window::addChar(char ch) {
  str += ch;
  cursor++;
  waddch(w, ch);
}
void Window::addString(std::string str) {
  this->str.insert(cursor, str);
  cursor += str.size();
  wprintw(w, "%s\n", str.c_str());
  //   wrefresh();
}

std::string Window::popString() {
  auto string = str;
  str.clear();
  werase(w);
  box(w, 0, 0);
  wmove(w, 1, 1);
  //   wrefresh();
  return string;
}

void Window::delChar() {  // test needed
  int x, y;
  getyx(w, y, x);
  if (x < 2) return;
  //   const char space = ' ';
  //   mvwaddnstr(w, y, x, &space, 1);
  //   wmove(w, y, x);
  //   wrefresh();
  wdelch(w);
  str.erase(cursor, 1);
}

void Window::backChar() {
  int x, y;
  getyx(w, y, x);
  if (x < 2) return;
  //   const char space = ' ';
  //   mvwaddnstr(w, y, x - 1, &space, 1);
  //   wmove(w, y, x - 1);
  //   wrefresh();
  wmove(w, y, x - 1);
  wdelch(w);
  cursor--;
  str.erase(cursor, 1);
}

int Window::getch() { return wgetch(w); }

void Window::keyUp() {}
void Window::keyDown() {}
void Window::keyLeft() {
  int x, y;
  getyx(w, y, x);
  if (x == 1) {
    if (y > 1) {
      wmove(w, y - 1, x);
      cursor--;
    }
    return;
  }
  wmove(w, y, x - 1);
  cursor--;
}
void Window::keyRight() {
  int x, y;
  getyx(w, y, x);
  if (x == width - 1) {
    if (y < height - 1) {
      wmove(w, y + 1, 1);
      cursor++;
    }
    return;
  }
  wmove(w, y, x - 1);
  cursor++;
}