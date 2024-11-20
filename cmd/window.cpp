#include "window.h"

#include <algorithm>
#include <format>
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

void Window::refreshBox() {
  box(wbox, 0, 0);
  if (!name.empty()) mvwprintw(wbox, 0, 1, name.c_str());
}

void Window::touch() {
  touchwin(wbox);
  touchwin(w);
}

void Window::untouch() {
  untouchwin(wbox);
  untouchwin(w);
}

void Window::clear() {
  str.clear();
  cursor = 0;
  strREV = false;
  wclear(w);
}

void Window::addChar(char ch) {  // TODO: newline show error
  str.insert(cursor, 1, ch);

  int in = ch;
  // waddnstr(w,&ch,1);
  if (strREV) in |= A_REVERSE;
  if (cursor + 1 == str.size()) {
    waddch(w, in);
    cursor++;
  } else {
    winsch(w, in);
    keyRight();
  }
}

void Window::newLine() {
  str.insert(cursor, 1, '\n');
  int y = getcury(w);
  auto c=str[cursor];
  // winstr(w,buffer.data());
  wclrtoeol(w);
  waddnstr(w, str.data() + cursor, str.size() - cursor);
  mvwchgat(w, y, 0, -1, A_NORMAL, 1, NULL);
  mvwchgat(w, ++y, 0, -1, strREV?A_REVERSE:A_NORMAL, 1, NULL);
  cursor++;
}

void Window::addString(std::string str) {
  // int y, x;
  // getyx(w, y, x);

  // rawLine += std::ranges::count(str, '\n');
  int pp, p, size;
  size = str.size();
  this->str.insert(cursor, str);
  cursor += size;
  waddnstr(w, str.data(), size);
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
  cursor--;
}

void Window::backChar() {
  int x, y;
  getyx(w, y, x);
  if (x < 1) return;
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
  int y, x, s;
  getyx(w, y, x);
  s = str.size();
  if (y == 0) return;  // TODO: scroll up
  int p = str.rfind('\n', cursor);
  int pp = str.rfind('\n', p - 1);
  int preLen = p - pp;

  int col = (preLen > x) ? x : preLen - 1;
  cursor = pp + col + 1;
  mvwchgat(w, y, 0, -1, A_NORMAL, 1, NULL);
  mvwchgat(w, --y, 0, -1, A_REVERSE, 1, NULL);
  wmove(w, y, col);
  strREV = true;
}

void Window::keyDown() {
  // cursor unstopable
  int y, x;
  getyx(w, y, x);
  if (y == height - 3) return;  // TODO: scroll down
  int p = str.find('\n', cursor);
  if (p < 0) return;
  int n = str.find('\n', p + 1);
  if (n < 0) n = str.size() - 1;
  int curLen = n - p;

  int col = (curLen > x) ? x : curLen - 1;  // '\n'
  cursor = p + col + 1;
  mvwchgat(w, y, 0, -1, A_NORMAL, 1, NULL);
  mvwchgat(w, ++y, 0, -1, A_REVERSE, 1, NULL);
  wmove(w, y, col);
  strREV = true;
}

void Window::keyLeft() {
  int x, y, preLen;
  if (cursor == 0) return;
  getyx(w, y, x);

  if (str[--cursor] != '\n') {
    wmove(w, y, x - 1);
    auto c = str[cursor];
    return;
  }
  int p = str.rfind('\n', cursor);
  int pp = str.rfind('\n', p - 1);
  preLen = p - pp;
  if (strREV) {
    mvwchgat(w, y, 0, -1, A_NORMAL, 1, NULL);
    mvwchgat(w, --y, 0, -1, A_REVERSE, 1, NULL);
  }
  wmove(w, y, preLen);
  auto cc = str[cursor];
}

void Window::keyRight() {
  int x, y;
  if (cursor >= str.size()) return;
  getyx(w, y, x);
  // int p = str.find('\n', cursor);
  if (str[cursor++] != '\n') {
    wmove(w, y, x + 1);
    auto c = str[cursor];
    return;
  }
  if (strREV) {
    mvwchgat(w, y, 0, -1, A_NORMAL, 1, NULL);
    mvwchgat(w, ++y, 0, -1, A_REVERSE, 1, NULL);
  }
  wmove(w, y, 0);
  auto c = str[cursor];
}

void Window::printCurStr(std::string str) {
  // TODO: unreset cursor and this->str
  int y, x;
  getyx(w, y, x);
  std::string s = std::format("({:>3}, {:>3})", y, x);
  wattron(wbox, COLOR_PAIR(2));
  mvwaddnstr(wbox, 0, width - 11, s.data(), s.size());
  // mvwprintw(wbox, 0, width - 11, s.c_str());
  wattroff(wbox, COLOR_PAIR(2));
  // ::wrefresh(wbox);
}

int Window::getline() { return getcury(w); }