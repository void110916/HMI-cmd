#include "window.h"

#include <algorithm>
std::vector<Window *> Window::windows;

Window::~Window() {}

Window::Window(int height, int width, int startY, int startX, std::string name,
               bool leavok)
    : height(height), width(width), startY(startY), startX(startX) {
  wbox = newwin(height, width, startY, startX);
  box(wbox, 0, 0);

  w = derwin(wbox, height - 2, width - 2, 1, 1);
  // box(w,0,0);
  nodelay(w, true);
  keypad(w, TRUE);
  intrflush(w, TRUE);

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

int Window::refresh() { return ::refresh(); }

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
    wsetscrreg(w, 0, height - 3);
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

void Window::clear() { wclear(w); }

void Window::addChar(char ch) {
  str += ch;
  cursor++;
  waddch(w, ch);
}
void Window::addString(std::string str) {
  int y, x;
  getyx(w, y, x);

  rawLine += std::ranges::count(str, '\n');
  this->str.insert(cursor, str);
  cursor += str.size();
  waddnstr(w, str.data(), str.size());
}

std::string Window::popString() {
  auto string = str;
  str.clear();
  werase(w);  // TODO: change to clear()?
  wmove(w, 0, 0);
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

void Window::keyUp() {
  int y, x;
  getyx(w, y, x);
  if (y == 0) return;
  int pp = str.rfind('\n', cursor - 2);
  if (pp < 0) pp = 0;
  int p = str.rfind('\n', cursor);
  int col = (p - pp > x) ? x : p - pp;
  mvwchgat(w, y, col, -1, A_NORMAL, 1, NULL);
  // rawLine--;
  --y;
  mvwchgat(w, y, col, -1, A_REVERSE, 1, NULL);
  cursor = pp;
}

void Window::keyDown() {
  // cursor unstopable
  int y, x;
  getyx(w, y, x);
  if (y == height - 3) return;
  // int pp = str.find('\n', cursor - 2);
  int p = str.find('\n', cursor);
  int n = str.find('\n', p+1);
  if (n < 0) n = str.size();
  auto c = str[p + 1];
  int num=str.size();
  if (p + 1 >= str.size()) p=n=str.size();
  int col = (n - p > x) ? x : n - p;
  mvwchgat(w, y, col, -1, A_NORMAL, 1, NULL);
  // rawLine++;
  y++;
  mvwchgat(w, y, col, -1, A_REVERSE, 1, NULL);
  cursor = p + col+1;  // maybe +1?
}

void Window::keyLeft() {
  int x, y;
  getyx(w, y, x);
  if (x == 0) {
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
  auto size = str.size();
  if (cursor == size) {
    if (y < height - 3) {
      wmove(w, y + 1, 0);
      cursor++;
    }
    return;
  }
  wmove(w, y, x + 1);
  cursor++;
}

int Window::getline() { return getcury(w); }