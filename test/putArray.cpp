#include <fstream>
#include <iostream>
#include <string>

#include "HMI.h"
#include "serialPort.h"

using std::cout;
using std::ifstream;
using std::string;

int main(int argc, char *argv[]) {
  ifstream f;
  cout <<"test put array"<<std::endl;
  f.open("../../test_txt/i8_5.txt");
  if (f.fail()) {
    cout << "file no found" << std::endl;
    return 1;
  }
  TimeoutSerial com("COM4", 38400);
  com.setTimeout(boost::posix_time::seconds(1000));
  if (!com.isOpen()) {
    cout << "COM port not open" << std::endl;
  }
  string str;
  f >> str;
  f.close();
  com.writeString("1\n");
  HMI_put_array(com, str);
  cout << com.readStringUntil() << std::endl;
  cout << "end test" << std::endl;
  com.close();

  return 0;
}
