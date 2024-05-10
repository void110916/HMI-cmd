#include <iostream>

#include "asaEncoder.h"
#include "serialPort.h"

using namespace std;

int main() {
  std::cerr << "start\n" <<std::flush;
  TimeoutSerial com;
  try
  {
    com.open("COM4", 38400);
    com.setTimeout(boost::posix_time::seconds(5));
  }
  catch(const std::exception& e)
  {
    std::cerr << e.what() << '\n';
  }
  
  
  // serialPort com("COM4");
  // // com.setTimeout(boost::posix_time::seconds(5));
  // // if (!com.isOpen()) {
  // //   std::cout << "serial open fail !!!" << std::endl;
  // //   return 1;
  // // }
  while (1) {
    string str = com.readString(1);
    if (!str.empty()) {
      std::cout << str << std::flush;
      // break;
    }
    // cout<<"c"<<endl;
  }

  return 0;
}