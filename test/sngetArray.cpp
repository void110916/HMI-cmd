#include <fstream>
#include <iostream>
#include <string>

#include "HMI.h"
#include "serialPort.h"

using std::cout;
using std::ifstream;
using std::string;

int main(int argc, char *argv[]) {
  //   ifstream f;
  //   f.open("../../test_txt/i8_5.txt");
  //   if (f.fail()) {
  //     cout << "file no found" << std::endl;
  //     return 1;
  //   }
  cout << "test get Array\n" << std::endl;
  TimeoutSerial com("COM4", 38400);
  com.setTimeout(boost::posix_time::seconds(5));
  if (!com.isOpen()) {
    cout << "COM port not open" << std::endl;
  }

  string str;
  // com.writeString("1\n");
  if (HMI_snget_array(com, str)) {
    cout << "error get\n" << std::endl;
    return 1;
  }
  // string c = com.readStringUntil();
  cout << str << std::endl;
  // cout << c << std::endl;
  cout << "end test\n" << std::endl;
  com.close();

  return 0;
}
