#include <cxxopts.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include "asaEncoder.h"
#include "serialPort.h"
// #include "HMI.h"
#include "window.h"

#include "edit.h"

using namespace std;
// #define SCORE_SIZE 3
#define ISCTRL(ch) (ch & 0x1f)

int cmd_decode(string str);
void change_win();

Window ltwin, lbwin, rtwin, rbwin, *focus;

// BufferedAsyncSerial serial;
SerialPort serial;

int main(int argc, char *argv[]) {
  cxxopts::Options opt("info");

  opt.add_options()("h,help", "Help info")
  ("p,port", "serial port",cxxopts::value<string>(), "NAME")
  ("b,baud", "serial port baud rate",
    cxxopts::value<int>()->default_value("38400"),"NUM");
  // ("t,time", "serial port timeout (s)",
  //   cxxopts::value<int>()->default_value("0"),"NUM");

  auto res = opt.parse(argc, argv);
  if (res.count("help")) {
    cout << opt.help() << endl;
    exit(0);
  }
  string comStr;
  int baud;
  // int time;
  // get serial args
  try {
    comStr = res["p"].as<string>();
    baud = res["b"].as<int>();
    // time = res["t"].as<int>();
    // cout << baud << endl;
  } catch (const cxxopts::exceptions::exception &e) {
    cout << e.what() << endl;
    exit(1);
  }
  // =======================
  // open serial with args
  serial.open(comStr,baud);
  // serial.setTimeout(time);
  if(!serial.isOpen()){
    cout<< "serial \""<<"comStr"<<"\" open fail"<<endl;
    exit(1);
  }
  // ======================
  // init global window
  initscr();
  // ======================
  //   set keyboard property
  curs_set(1);
  keypad(stdscr,TRUE);
  raw();  // disable signal (ex. ctrl+c)
  noecho();
  nodelay(stdscr, true);
  intrflush(stdscr, true);
  refresh();
  // ======================
  // init sub window

  /// ltwin = newwin(LINES - SCORE_SIZE, COLS / 2, 0, 0);
  /// rtwin = newwin(LINES, COLS / 2, 0, COLS / 2);
  /// lbwin = newwin(SCORE_SIZE, COLS / 2, LINES - SCORE_SIZE, 0);
  /// rbwin = newwin(LINES - SCORE_SIZE, COLS / 2, 0, COLS / 2);

  /// box(ltwin, 0, 0);
  /// mvwprintw(ltwin, 0, 1, " port terminal ");
  /// wmove(ltwin, 1, 1);
  /// scrollok(ltwin, true);
  /// wsetscrreg(ltwin, 1, LINES - SCORE_SIZE);
  /// wrefresh(ltwin);

  /// box(lbwin, 0, 0);
  /// wmove(lbwin, 1, 1);
  /// wrefresh(lbwin);

  /// box(rtwin, 0, 0);
  /// mvwprintw(rtwin, 0, 1, " workspace ");
  /// wrefresh(rtwin);

  // refresh();

  // WINDOW *ltbox=subwin(stdscr,LINES- SCORE_SIZE,COLS/2,0,0);
  // WINDOW *lbbox=subwin(stdscr,SCORE_SIZE,COLS/2,LINES- SCORE_SIZE,0);
  // // touchwin(stdscr);
  // // refresh();
  // WINDOW *rtbox=subwin(stdscr,LINES,COLS/2,0,COLS/2);
  // box(ltbox,0,0);
  // mvwprintw(ltbox, 0, 1, " port terminal ");
  // box(lbbox,0,0);
  // box(rtbox,0,0);
  // mvwprintw(rtbox, 0, 1, " workspace ");
  // touchwin(stdscr);
  // refresh();

  ltwin = Window(LINES - SCORE_SIZE, COLS / 2, 0, 0, " port terminal ",true);
  rbwin = Window(SCORE_SIZE, COLS / 2, LINES - SCORE_SIZE, COLS / 2);
  rtwin = Window(LINES, COLS / 2, 0, COLS / 2, " workspace ");
  lbwin = Window(SCORE_SIZE, COLS / 2, LINES - SCORE_SIZE, 0);
  focus= &lbwin;

  ltwin.waitUpdate();
  rtwin.waitUpdate();
  lbwin.waitUpdate();
  Window::update();
  // ======================

  // string lstr, rstr;
  bool putSync=false;
  while (1) {
    // read serial
    string s;
    // focus->wrefresh();
    auto chars = serial.readAsync(256,5).get();
    if(!chars.empty()){
      for (const char ch:chars){
        static ASAEncoder::ASADecode decode;
        if (!decode.put(ch)) {
          if (ch == '\r') continue;
          if (decode.isSync(ch)) {
            serial.writeAsync("~ACK\n");
            s+="~ACK\n";
          }
          putSync=ASAEncoder::ASAEncode::isSync(ch);
          ltwin.addChar(ch);
        }
        if(decode.isDone){
          // add struct object
          // add struct object to window
        }
      }
      // ltwin.touch();
      ltwin.waitUpdate();
      Window::update();
    }    
    // ======================
    // verify key 
    int key = focus->getch();
    if (key != ERR) {
      switch (key) {
        case ISCTRL('o'):  // ESC
        {
          endwin();
          if(serial.isOpen()) serial.close();
          return 0;
          break;
        }
        case '\n':  // enter //old: 10
        {
          /// wprintw(ltwin," %s\n", lstr.c_str());
          /// box(ltwin, 0, 0);
          /// wrefresh(ltwin);
          /// werase(lbwin);
          /// box(lbwin, 0, 0);
          /// wmove(lbwin, 1, 1);
          /// wrefresh(lbwin);
          /// lstr.clear();

          // left event =============
          string str=lbwin.popString();
          lbwin.waitUpdate();
          ltwin.addString(str);
          ltwin.waitUpdate();
          // ====================
          break;
        }
        case KEY_BACKSPACE:  // backspace
        {
          /// int x, y;
          /// getyx(lbwin, y, x);
          /// if (x < 2) break;
          /// const char space = ' ';
          /// mvwaddnstr(lbwin, y, x - 1, &space, 1);
          /// wmove(lbwin, y, x - 1);
          /// wrefresh(lbwin);
          /// lstr.pop_back();

          focus->backChar();
          focus->waitUpdate();
          break;
        }
        case KEY_DC: // delete
          focus->delChar();
          focus->waitUpdate();
          break;
        case KEY_DOWN:
          focus->keyDown();
          focus->waitUpdate();
          break;
        case KEY_UP:
          focus->keyUp();
          focus->waitUpdate();
          break;
        case KEY_LEFT:
          focus->keyLeft();
          focus->waitUpdate();
          break;
        case KEY_RIGHT:
          focus->keyRight();
          focus->waitUpdate();
          break;
        case '\t':
          change_win();
          break;
        default:
              //   waddch();
          /// waddnstr(lbwin, &ch,1);
              //   wprintw(lbwin, &ch);
          /// wrefresh(lbwin);
          /// lstr.push_back(ch);
              //   str[pCur++] = ch;

          // lbwin, rbwin event
          char c;
          c=key&0xff;
          focus->addChar(c);
          // focus->wrefresh();
          focus->waitUpdate();
          break;
      }
      Window::update();
    }
    
    this_thread::sleep_for(chrono::milliseconds(100));
  }

  // pause the screen output

  // deallocates memory and ends ncurses
  endwin();
  return 0;
}

// enum CmdArg{

// }

int cmd_decode(string str){
  cxxopts::Options opt("cmd args");

  opt.add_options()
  ("m,mode","HMI communicate mode:\n"
            "\tsngarr, sngmat, sngstr,\n"
            "\tsnparr, snpmat, snpstr",
                    cxxopts::value<string>())
  ("s,save", "save file",cxxopts::value<string>(), "FILE")
  ("l,load", "load file", cxxopts::value<string>(), "FILE");
  // ("list","list all");
  map<string,int> CmdArg{
    {"-m",1},
    {"--mode",1},
    {"-s",2},
    {"--save",2},
    {"-l",3},
    {"--load",3}
    };
  map<string,int> ModeArg{
    {"snparr",1},
    {"snpmat",2},
    {"snpstr",3},
    };
  // vector<string> strs;
  string s;
  while (getline(stringstream(str),s,' '))
  {
    int a=CmdArg[s];
    // switch (a)
    // {
    // case 1: // mode
    //   s++;
    //   if(ModeArg[s]==1)
    //     HMI_snget_array();
    //   else if(ModeArg[s]==2)
    //     // {
    //     //   // HMI_snget_
    //     // }
    //   else if(ModeArg[s]==3)
    //     {}

    //   break;
    // case 2: // save
    // s++;
    // auto f = fopen(s,"a");
    
    // fwrite()
    // case 3: // load
    // default:
    //   break;
    // }
  }
  
  // boost::split(strs,str.c_str(),[](char c){return ' ';});
  // auto res = opt.parse(strs.size(),strs.data());
  // for (auto& s:strs)
  // {
    int a=CmdArg[s];
    // switch (a)
    // {
    // case 1: // mode
    //   s++;
    //   if(ModeArg[s]==1)
    //     HMI_snget_array();
    //   else if(ModeArg[s]==2)
    //     // {
    //     //   // HMI_snget_
    //     // }
    //   else if(ModeArg[s]==3)
    //     {}

    //   break;
    // case 2: // save
    // s++;
    // auto f = fopen(s,"a");
    
    // fwrite()
    // case 3: // load
    // default:
    //   break;
    // }
  // }
  
  
    return 0;
}





// #define NCURSES_NOMACROS
// #include <ncursesw/ncurses.h>

// #include <cxxopts.hpp>
// #include <iostream>
// #include <string>
// #include <vector>
// // #include "asaEncoder.h"
// #include "serialPort.h"
// #include "HMI.h"
// #include <boost/algorithm/string.hpp>

// using namespace std;
// #define SCORE_SIZE 3
// #define ISCTRL(ch) (ch & 0x1f)

// int cmd_decode(string str);
// void change_rbwin();

// WINDOW *ltwin, *lbwin, *rtwin, *rbwin, *focus;

// TimeoutSerial *serial;
// // WINDOW *cursor;
// int main(int argc, char *argv[]) {
//   cxxopts::Options opt("info");

//   opt.add_options()("h,help", "Help info")
//   ("p,port", "serial port",cxxopts::value<string>(), "NAME")
//   ("b,baud", "serial port baud rate",
//     cxxopts::value<int>()->default_value("38400"),"NUM")
//   ("t,time", "serial port timeout (s)",
//     cxxopts::value<int>()->default_value("10"),"NUM");

//   auto res = opt.parse(argc, argv);
//   if (res.count("help")) {
//     cout << opt.help() << endl;
//     exit(0);
//   }
//   string comStr;
//   int baud;
//   int time;

//   try {
//     comStr = res["p"].as<string>();
//     baud = res["b"].as<int>();
//     time = res["t"].as<int>();
//     cout << baud << endl;
//   } catch (const cxxopts::exceptions::exception &e) {
//     std::cout << e.what() << endl;
//     exit(1);
//   }
//   serial=new TimeoutSerial(comStr,baud);
//   serial->setTimeout(time);
//   initscr();
//   curs_set(0);

//   //   set keyboard property
//   raw();  // disable signal (ex. ctrl+c)
//   noecho();
//   nodelay(stdscr, true);
//   intrflush(stdscr, true);
//   // ------------------------

//   ltwin = newwin(LINES - SCORE_SIZE, COLS / 2, 0, 0);
//   rtwin = newwin(LINES, COLS / 2, 0, COLS / 2);
//   lbwin = newwin(SCORE_SIZE, COLS / 2, LINES - SCORE_SIZE, 0);
//   //   WINDOW *rbwin = newwin(LINES - SCORE_SIZE, COLS / 2, 0, COLS / 2);
//   // refreshes the screen
//   //   move(LINES - SCORE_SIZE + 1, 1);
//   refresh();

//   // clearok(ltwin,true);
//   box(ltwin, 0, 0);
//   mvwprintw(ltwin, 0, 1, " port terminal ");
//   wmove(ltwin, 1, 1);
//   scrollok(ltwin, true);
//   wsetscrreg(ltwin, 1, LINES - SCORE_SIZE);
//   wrefresh(ltwin);

//   box(lbwin, 0, 0);
//   wmove(lbwin, 1, 1);
//   wrefresh(lbwin);

//   box(rtwin, 0, 0);
//   mvwprintw(rtwin, 0, 1, " workspace ");
//   wrefresh(rtwin);

//   refresh();
//   string lstr, rstr;
//   while (1) {
//     int ch = getch();
//     if (ch != ERR) {
//       switch (ch) {
//         case ISCTRL('o'):  // ESC
//         {
//           endwin();
//           return 0;
//           break;
//         }
//         case KEY_ENTER:  // enter
//         {
//             // lstr.push_back('\n');
//           wprintw(ltwin," %s\n", lstr.c_str());
//           box(ltwin, 0, 0);
//           wrefresh(ltwin);
//           werase(lbwin);
//           box(lbwin, 0, 0);
//           wmove(lbwin, 1, 1);
//           wrefresh(lbwin);
//           lstr.clear();
//           break;
//         }
//         case KEY_BACKSPACE:  // backspace
//         {
//           int x, y;
//           getyx(lbwin, y, x);
//           if (x < 2) break;

//           //   wdelch(lbwin);
//           //   mvwaddch(lbwin,y,x-2,' ');
//           const char space = ' ';
//           mvwaddnstr(lbwin, y, x - 1, &space, 1);
//           wmove(lbwin, y, x - 1);
//           wrefresh(lbwin);
//           lstr.pop_back();
//           break;
//         }
//         case KEY_DL:
//         int x, y;
//           getyx(lbwin, y, x);
//           const char space = ' ';
//           mvwaddnstr(lbwin, y, x , &space, 1);
//           wrefresh(lbwin);
//           lstr.pop_back();
//         break;
//         case KEY_STAB:
        
//         default:
//           //   waddch();
//           char c=ch&0xff;
//             waddnstr(lbwin, &c,1);
//         //   wprintw(lbwin, &ch);
//           wrefresh(lbwin);
//           lstr.push_back(ch);
//           //   str[pCur++] = ch;

//           break;
//       }
//     }
//   }

//   // pause the screen output

//   // deallocates memory and ends ncurses
//   endwin();
//   return 0;
// }

// // enum CmdArg{

// // }

// int cmd_decode(string str){
//   cxxopts::Options opt("cmd args");

//   opt.add_options()
//   ("m,mode","HMI communicate mode:\n"
//             "\tsngarr, sngmat, sngstr,\n"
//             "\tsnparr, snpmat, snpstr",
//                     cxxopts::value<string>())
//   ("s,save", "save file",cxxopts::value<string>(), "FILE")
//   ("l,load", "load file", cxxopts::value<string>(), "FILE");
//   // ("list","list all");
//   map<string,int> CmdArg{
//     {"m",1},
//     {"mode",1},
//     {"s",2},
//     {"save",2},
//     {"l",3},
//     {"load",3}
//     };
//   map<string,int> ModeArg{
//     {"snparr",1},
//     {"snpmat",2},
//     {"snpstr",3},
//     };
//   vector<char *> strs;
//   boost::split(strs,str.c_str(),[](char c){return ' ';});
//   auto res = opt.parse(strs.size(),strs.data());
//   for (auto& s:strs)
//   {
//     int a=CmdArg[s];
//     switch (a)
//     {
//     case 1: // mode
//       s++;
//       // if(ModeArg[s]==1)
//       //   HMI_snget_array();
//       // else if(ModeArg[s]==2)
//       //   // {
//       //   //   // HMI_snget_
//       //   // }
//       // else if(ModeArg[s]==3)
//       //   {}

//       break;
//     // case 2: // save
//     // s++;
//     // auto f = fopen(s,"a");
//     // // fwrite()
//     // break;
//     // case 3: // load
//     // break;
//     default:
//       break;
//     }
//   }
  
//     try {
//       // comStr = res["p"].as<string>();
//       // baud = res["b"].as<int>();
//       // time = res["t"].as<int>();
//       // cout << baud << endl;
//     } catch (const cxxopts::exceptions::exception &e) {
//       wprintw(ltwin," >>[W]%s\n", e.what());
//     }
//     return 0;
// }

// void change_rbwin(bool show){
//   if(show)
//   {
//     rtwin = newwin(LINES-SCORE_SIZE, COLS / 2, 0, 0);
//     box(rtwin, 0, 0);
//     mvwprintw(rtwin, 0, 1, " workspace ");
//     wrefresh(rtwin);

//     rbwin = newwin(SCORE_SIZE, COLS / 2, LINES - SCORE_SIZE, 0);
//     box(rbwin, 0, 0);
//     wmove(rbwin, 1, 1);
//   }
//   else
//   {
//     rtwin = newwin(LINES-SCORE_SIZE, COLS / 2, 0, 0);
//     box(rtwin, 0, 0);
//     mvwprintw(rtwin, 0, 1, " workspace ");
//   }
  
//   wrefresh(rbwin);
//   // refresh();
// }