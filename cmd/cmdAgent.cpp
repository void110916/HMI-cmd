#include <iostream>
#include <string>
#include <cxxopts.hpp>
#include "HMI.h"

using namespace std;

int main(int argc, char *argv[]){
    cxxopts::Options opt("info");

    opt.add_options()
        ("h,help", "Help info")
        ("p,port", "serial port", cxxopts::value<string>(),"NAME")
        ("b,baud", "serial port baud rate", cxxopts::value<int>()->default_value("38400"),"NUM")
        ("t,time", "serial port timeout (s)", cxxopts::value<int>()->default_value("10"),"NUM")
        ("m,mode", "HMI communicate mode:\n"
                    "\tsngarr, sngmat, sngstr,\n"
                    "\tsnparr, snpmat, snpstr"
        , cxxopts::value<string>())
        ("s,save", "save file", cxxopts::value<string>(),"FILE")
        ("l,load", "load file", cxxopts::value<string>(),"FILE")
        ("list", "list all")
    ;

    auto res=opt.parse(argc,argv);
    if(res.count("help"))
    {
        cout<<opt.help()<<endl;
        exit(0);
    }
    string comStr;
    int baud;
    int time;

    try
    {
        comStr=res["p"].as<string>();
        baud=res["b"].as<int>();
        time=res["t"].as<int>();
        cout<< baud<<endl;
    }
    catch(const cxxopts::exceptions::exception& e)
    {
        std::cout << e.what() << endl;
        exit(1);
    }

    return 0;
}