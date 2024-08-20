#define NCURSES_NOMACROS
#include <ncursesw/ncurses.h>

#include <string>
#define SCORE_SIZE 3
class Window {
 private:
  WINDOW *w;
  int height, width, startY, startX;
  std::string name;
  std::string str;
  bool showOnly;
  int cursor = 0;

 public:
  Window(){}
  Window(int height, int width, int startx, int starty, std::string name = "");
  ~Window();
  int wrefresh();
  static int refresh();
  void resize(int height, int width);

  void addString(std::string str);
  std::string popString();
  void delChar();
  void backChar();

  void keyUp();
  void keyDown();
  void keyLeft();
  void keyRight();
};

Window::Window(int height, int width, int startY, int startX, std::string name)
    : height(height), width(width), startY(startY), startX(startX) {
  w = newwin(height, width, startY, startX);
  box(w, 0, 0);
  wmove(w, 1, 1);

  if (!name.empty()) {
    this->name = name;
    mvwprintw(w, 0, 1, name.c_str());
    scrollok(w, true);
    wsetscrreg(w, 1, height);
  }
  wrefresh();
}

Window::~Window() {}

int Window::wrefresh() { return ::wrefresh(w); }

int Window::refresh() { return ::refresh(); }

void Window::resize(int height, int width) {
  this->height = height;
  this->width = width;
  wresize(w,height,width);
//   w = newwin(height, width, startY, startX);
//   box(w, 0, 0);
//   wmove(w, 1, 1);

  if (!name.empty()) {
    this->name = name;
    mvwprintw(w, 0, 1, name.c_str());
    scrollok(w, true);
    wsetscrreg(w, 1, height);
  }
//   wrefresh();
}

void Window::addString(std::string str) {
  this->str.insert(cursor, str);
  cursor += str.size();
  wprintw(w, " %s\n", str.c_str());
  box(w, 0, 0);
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

void Window::keyUp(){

}
void Window::keyDown(){}
void Window::keyLeft(){
    int x, y;
    getyx(w, y, x);
    if (x == 1) {
        if(y>1){
            wmove(w, y-1, x);
            cursor--;
        }
        return;
    }
    wmove(w, y, x - 1);
    cursor--;
}
void Window::keyRight(){
    int x, y;
    getyx(w, y, x);
    if (x == width-1) {
        if(y<height-1){
            wmove(w, y+1, 1);
            cursor++;
        }
        return;
    }
    wmove(w, y, x - 1);
    cursor++;
}