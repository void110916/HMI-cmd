#define NCURSES_NOMACROS
#include <ncursesw/ncurses.h>
#include "serialPort.h"

int main(){
    initscr();
    curs_set(1);
    nodelay(stdscr, true);
    nonl();
    noecho();
    refresh();
    WINDOW *w=newwin(LINES,COLS,0,0);
    scrollok(w,TRUE);
    box(w,0,0);
    wmove(w,1,1);
    wrefresh(w);
    BufferedAsyncSerial serial;
    serial.open("COM4",38400);
    while (1)
    {
        auto str=serial.read();
        
        if (!str.empty())
        {
            // wprintw(w,"%s",str);
            // addnstr(str.data(),str.size());  
            /// these two above can't print cause CR by serial
            for(auto c:str){
                if(c=='\r') continue;  // skip CR
                // waddnstr(w,&c,1);
                wprintw(w,"%c",c);
                wrefresh(w);
                }
            
            wrefresh(w);
        }
        int key = getch();
        if (key != ERR) {
            waddch(w,key);
            wrefresh(w);
        }
        usleep(1000);
    }
    return 0;
}