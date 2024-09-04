#ifndef WINODW_H
#define WINDOW_H
#define NCURSES_NOMACROS
#include <ncursesw/ncurses.h>

#include <string>
#include <vector>
#define SCORE_SIZE 3
class Window {
 private:
  static std::vector<Window *> windows;

  int height, width, startY, startX;
  std::string name;
  std::string str;
  bool showOnly;
  int cursor = 0;
  int rawLine = 0;

 public:
  WINDOW *w;
  WINDOW *wbox;

  Window() {}
  Window(int height, int width, int startx, int starty,
         std::string scrollable = "", bool leavok = false);
  ~Window();
  int wrefresh();
  /**
   * @brief
   *
   */
  void waitUpdate();
  static int refresh();
  /**
   * @brief Update windows called by waitUpdate()
   *
   */
  static void update();
  static void updateAll();
  void resize(int height, int width);
  void touch();
  void untouch();
  void addChar(char ch);
  void addString(std::string str);
  std::string popString();
  void delChar();
  void backChar();
  int getch();

  void keyUp();
  void keyDown();
  void keyLeft();
  void keyRight();
  int getline();
};

#endif  // WINDOW_H