#include "HMI.h"

using ASAEncoder::ASADecode;
using ASAEncoder::ASAEncode;

uint8_t HMI_put_array(TimeoutSerial& serial, std::string& str) {
  ASAEncode encode;

  // ...
  encode.put(str);
  auto pac = encode.get();
  serial.write(reinterpret_cast<char*>(pac.data()), pac.size());
  return 0;
}

uint8_t HMI_get_array(TimeoutSerial& serial, std::string& str) {
  ASADecode decode;
  // std::string buf = serial.readStringUntil();
  // // ...
  // int o='\n';
  do {
    auto c = HMI_getc(serial);
    decode.put(c);
  } while (!decode.isDone);

  // for (auto i = buf.begin(); i < buf.end(); i++) {
  //   decode.put(*i);
  // }
  // if (!decode.isDone) return 1;
  str = decode.get();
  return 0;
}

uint8_t HMI_snget_array(TimeoutSerial& serial, std::string& str) {
  ASADecode decode;
  char c;
  do {
    // try {
      c = HMI_getc(serial);
    // } catch (const std::exception& e) {
    //   return 1;
    // }

    if (!decode.put(c)) {
      if (decode.isSync(c)) serial.writeString("~ACK\n");
    }
  } while (!decode.isDone);

  // for (auto i = buf.begin(); i < buf.end(); i++) {
  //   decode.put(*i);
  // }
  // if (!decode.isDone) return 1;
  str = decode.get();
  return 0;
}