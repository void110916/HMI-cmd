#include <iostream>

#include "asaEncoder.h"
#include "serialPort.h"

using namespace std;

int main() {
  TimeoutSerial com("COM4", 38400);
  // serialPort com("COM4");
  com.setTimeout(boost::posix_time::seconds(5));
  if (!com.isOpen()) {
    cout << "serial open fail !!!" << endl;
    return 1;
  }
  while (1) {
    string str = com.readStringUntil();
    if (!str.empty()) {
      cout << str << endl;
      // break;
    }
    // cout<<"c"<<endl;
  }

  return 0;
}