#include <chrono>
#include <cxxopts.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "HMI.h"
#include "asaEncoder.h"
#include "serialPort.h"
#include "window.h"

using namespace std;
// #define SCORE_SIZE 3
#define ISCTRL(ch) (ch & 0x1f)

int cmd_decode(string str);
void change_win();

Window ltwin, lbwin, rtwin, rbwin, *focus;

// BufferedAsyncSerial serial;
SerialPort serial;

bool putSync = false;

int main(int argc, char *argv[]) {
  cxxopts::Options opt("info");

  opt.add_options()("h,help", "Help info")("p,port", "serial port",
                                           cxxopts::value<string>(), "NAME")(
      "b,baud", "serial port baud rate",
      cxxopts::value<int>()->default_value("38400"), "NUM");
  auto res = opt.parse(argc, argv);
  if (res.count("help")) {
    cout << opt.help() << endl;
    exit(0);
  }
  string comStr;
  int baud;
  try {
    comStr = res["p"].as<string>();
    baud = res["b"].as<int>();
  } catch (const cxxopts::exceptions::exception &e) {
    cout << e.what() << endl;
    exit(1);
  }
  // =======================
  // open serial with args
  serial.open(comStr, baud);
  // serial.setTimeout(time);
  if (!serial.isOpen()) {
    cout << "serial \"" << "comStr" << "\" open fail" << endl;
    exit(1);
  }
  // ======================
  // init global window
  initscr();
  // enable color
  start_color();
  init_pair(2, COLOR_BLACK, COLOR_BLUE);  // 1 is A_REVERSE
  // init_pair(2,COLOR_BLACK,COLOR_BLUE);
  // ======================
  //   set keyboard property
  curs_set(1);
  keypad(stdscr, TRUE);
  raw();  // disable signal (ex. ctrl+c)
  noecho();
  nodelay(stdscr, true);
  intrflush(stdscr, true);
  refresh();
  // ======================
  // init sub window
  ltwin = Window(LINES - SCORE_SIZE, COLS / 2, 0, 0, " port terminal ", true);
  lbwin = Window(SCORE_SIZE, COLS / 2, LINES - SCORE_SIZE, 0);
  rbwin = Window(SCORE_SIZE, COLS / 2, LINES - SCORE_SIZE, COLS / 2);
  rtwin = Window(LINES, COLS / 2, 0, COLS / 2, " workspace ");
  focus = &lbwin;

  ltwin.waitUpdate();
  rtwin.waitUpdate();
  lbwin.waitUpdate();
  Window::update();
  // ======================

  // string lstr, rstr;

  Object::setCol(COLS / 2 - 2);
  static bool firstPage = true;
  while (1) {
    // read serial =============================
    string s;
    auto chars = serial.readAsync(256, 5).get();
    if (!chars.empty()) {
      for (const char ch : chars) {
        static ASAEncoder::ASADecode decode;
        if (!decode.put(ch)) {
          if (ch == '\r') continue;
          ltwin.addChar(ch);
          if (decode.isSync(ch)) {
            string str = "~ACK\n";
            serial.writeAsync(str);
            ltwin.addString(str);
          }
          ltwin.waitUpdate();
          putSync = ASAEncoder::ASAEncode::isSync(ch);
        }
        if (decode.isDone) {
          // add struct object
          Object o((Object::TYPE)decode.getType(), decode.get());
          // add struct object to window (first page)
          if (firstPage) {
            string str = o.getVisible();
            rtwin.addString(str);
            rtwin.waitUpdate();
            focus->touch();
          }
        }
      }
      // ltwin.touch();

      Window::update();
    }
    // ======================
    // verify key ==================
    int key = focus->getch();
    int idx = 0;
    string str;
    static Object *o = nullptr;
    if (key != ERR) {
      switch (key) {
        case ISCTRL('o'):  // ESC
          endwin();
          if (serial.isOpen()) serial.close();
          return 0;
          break;
        case '\n':  // enter //old: 10
          // left event =============
          if (focus == &lbwin) {
            str = lbwin.popString() + "\n";
            serial.writeAsync(str);
            ltwin.addString(str);

            ltwin.waitUpdate();
            lbwin.waitUpdate();
          }
          // ====================
          // right top event========
          else if (focus == &rtwin) {
            if (firstPage) {  // first page event
              idx = focus->getline();
              o = Object::getObj(idx);
              if (o) {
                focus->clear();
                focus->addString(o->getName() + "\n");
                focus->printCurStr();
                // focus->addString( + "\n");
                focus->addString(o->getDetail());
                focus->waitUpdate();
              }
            } else {  // second page event
              if (focus->getline() == 0) {
                str = focus->popString();
                if (!o->change(str)) {
                  ltwin.addString(">> fail change " + o->getName()+"\n");
                }
                // jump back
                focus->clear();
                focus->addString(Object::getAllVisible());
                focus->waitUpdate();
              } else {
                focus->addChar('\n');
              }
            }
            focus->waitUpdate();
            firstPage = !firstPage;
          } else if (focus == &rbwin) {
            str = focus->popString();
            cmd_decode(str);
          }
          break;
        case KEY_BACKSPACE:  // backspace
          focus->backChar();
          focus->waitUpdate();
          break;
        case KEY_DC:  // delete
          focus->delChar();
          focus->waitUpdate();
          break;
        case KEY_DOWN:
          if (focus == &rtwin) {
            focus->keyDown();
            focus->waitUpdate();
          }
          break;
        case KEY_UP:
          if (focus == &rtwin) {
            focus->keyUp();
            focus->waitUpdate();
          }
          break;
        case KEY_LEFT:
          if (focus == &rtwin && firstPage) break;
          focus->keyLeft();
          focus->waitUpdate();
          break;
        case KEY_RIGHT:
          if (focus == &rtwin && firstPage) break;
          focus->keyRight();
          focus->waitUpdate();
          break;
        case '\t':
          change_win();
          break;
        default:
          // rtwin event
          if (focus == &rtwin && firstPage) break;
          // lbwin, rbwin event
          char c;
          c = key & 0xff;
          focus->addChar(c);
          focus->waitUpdate();
          break;
      }
      // ltwin.addString(std::format("{}  {}\n",focus->name,focus->cursor));
      ltwin.waitUpdate();
      focus->waitUpdate();
      Window::update();
    }

    this_thread::sleep_for(chrono::milliseconds(50));
  }

  // pause the screen output

  // deallocates memory and ends ncurses
  endwin();
  return 0;
}

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <fstream>
int cmd_decode(string str) {
  map<string, int> CmdArg{{"-m", 1},     {"--mode", 1}, {"-s", 2},
                          {"--save", 2}, {"-l", 3},     {"--load", 3}};
  map<string, int> ModeArg{
      // {"snparr", 1},
      {"snpmat", 2},
      {"snpstr", 3},
      {"snpfile", 4},
  };
  vector<string> strs;
  string s;
  boost::split(strs, str, boost::is_any_of(" "), boost::token_compress_on);
  // auto res = opt.parse(strs.size(),strs.data());
  for (auto s = strs.begin(); s != strs.end(); s++) {
    int a = CmdArg[*s];
    Object *o;
    string str;
    switch (a) {
      case 1: {  // mode
        s++;
        if (ModeArg[*s] == 4) {  // files
          // TODO
        } else {  // array, matrix, struct
          str = *(s + 1);
          o = Object::getObj(str);
          if (o != nullptr && serial.isOpen()) {
            ASAEncoder::ASAEncode enc;
            str = o->getFormat() + ":" + o->getDetail();
            if (enc.put(str)) {
              auto buf = enc.get();
              if (putSync) {
                str = "~ACK\n";
                serial.writeAsync(str);
                ltwin.addString(str);
                putSync = false;
              }
              serial.writeAsync(buf.data(), buf.size());
            }
          }
        }
        break;
      }
      case 2: {  // save
        s++;
        if (s == strs.end()) {  // save all objs

        } else {  // save single
        }
        fstream f;
        f.open(*s, ios::out);
        f << *s;
        break;
      }
      case 3:  // load
        break;
      default: {
        break;
      }
    }
  }

  return 0;
}

void change_win() {
  static int index = 0;
  // number: lbwin:0, rtwin:1, rbwin:2
  index = ++index % 3;
  if (index == 0) {
    focus = &lbwin;
    rtwin.resize(LINES, COLS / 2);
    rtwin.waitUpdate();
    // rtwin.wrefresh();
    focus->touch();
    focus->waitUpdate();
    // rtwin.wrefresh();
  } else if (index == 1) {
    focus = &rtwin;
    focus->waitUpdate();

  } else {
    rtwin.resize(LINES - SCORE_SIZE, COLS / 2);
    focus = &rbwin;
    focus->touch();
    rtwin.waitUpdate();
    rbwin.waitUpdate();
  }
}
