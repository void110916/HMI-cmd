#include <iostream>

// #include "asaEncoder.h"
#include "Serial.h"

using namespace std;

int main() {
  std::cerr << "start\n" <<std::flush;
  serial::Serial com;
  try
  {
    com.open("COM4", 38400);
    // com.setTimeout(boost::posix_time::seconds(5));
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
    auto str = com.receiveAsync(256,5).get();
    // auto str=future.get();
    if (!str.empty()) {
      std::cout << str.data() << std::flush;
      std::cout << "touch" << std::flush;
      // break;
    }
    // cout<<"c"<<endl;
    std::cout << "touch" << std::flush;
  }

  return 0;
}