#include "asaFormat.h"
namespace ASAEncoder {
enum FSTATE {
  f_ver = 41,
  f_type = 42,
  f_name_len = 43,
  f_name = 44,
  sector_num = 45,
  sector_idx = 46,
  sector_dat = 47
};

inline FSTATE& operator++(FSTATE& f, int) {
  switch (f) {
    case FSTATE::f_ver:
      return f = FSTATE::f_type;
    case FSTATE::f_type:
      return f = FSTATE::f_name_len;
    case FSTATE::f_name_len:
      return f = FSTATE::f_name;
    case FSTATE::f_name:
      return f = FSTATE::sector_num;
    case FSTATE::sector_num:
      return f = FSTATE::sector_idx;
    case FSTATE::sector_idx:
      return f = FSTATE::sector_dat;
    case FSTATE::sector_dat:
      return f = FSTATE::sector_idx;
    default:
      return f;
  }
}
union FPAC {
  struct PAC {
    array<uint8_t, 257> data;
    uint8_t chksum = 0;
  } pac;
  array<uint8_t, 258> raw;
  FPAC() : pac{0} {}
};
class ASAFileBasic {
 protected:
  enum FILETYPE {
    ASCII = 0,
    BIN = 1,
  };

 public:
  ASAFileBasic(/* args */);
  ~ASAFileBasic();
};

/**
 * @brief from PC to serial port
 *
 */
class ASAFileEnc : ASAFileBasic {
 private:
  /* data */
  FILETYPE ftype = FILETYPE::BIN;
  vector<uint8_t> header;
  vector<FPAC> fdata{0};
  bool isHeader = true;
  int idx = 0;

 public:
  ASAFileEnc(/* args */);
  ~ASAFileEnc();
  bool put(string str);
  vector<uint8_t> get(bool next = true);
  void clear();
};
/**
 * @brief from serial port to PC
 *
 */
class ASAFileDec : ASAFileBasic {
 private:
  FSTATE decodeState = FSTATE::f_ver;
  array<uint8_t, 256> buffer;
  uint16_t count = 0;
  string fileName;
  string str;
  uint8_t filetype;
  uint8_t sectorNum,sectorIdx;
  // bool isProcessing = false;
  bool skip = false;
  bool fend = false;

 public:
  ASAFileDec(/* args */);
  ~ASAFileDec();
  bool put(uint8_t buff);
  /**
   * @brief save buffer to file
   * 
   * @return true buffer is last buffer
   * @return false buffer is not last buffer
   */
  bool save();
  void clear();
};
}  // namespace ASAEncoder