#include <string>

#include "asaEncoder.h"
#include "serialPort.h"

static inline void HMI_put(TimeoutSerial& serial, std::string& str) {
  serial.writeString(str);
}

static inline char HMI_getc(TimeoutSerial& serial) { return serial.read(1)[0]; }

uint8_t HMI_put_array(TimeoutSerial& serial, std::string& str);

uint8_t HMI_get_array(TimeoutSerial& serial, std::string& str);

uint8_t HMI_snget_array(TimeoutSerial& serial, std::string& str);
