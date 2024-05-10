#include <array>
#include <cstdint>
#include <optional>
#include <regex>
#include <vector>
namespace ASAEncoder {
using std::array;
using std::optional;
using std::regex;
using std::smatch;
using std::ssub_match;
using std::string;
using std::vector;
using namespace std::string_literals;
enum HMI_type : uint8_t {
  I8 = 0,
  I16 = 1,
  I32 = 2,
  I64 = 3,
  UI8 = 4,
  UI16 = 5,
  UI32 = 6,
  UI64 = 7,
  F32 = 8,
  F64 = 9
};

enum PAC_type : uint8_t { AR = 1, MT = 2, ST = 3 };

enum class STATE : uint16_t {
  HEADER = 0,
  pkg_len = 1,
  switch_type = 2,
  chksum = 3,

  ar_type = 10,
  ar_num = 11,
  ar_dat_len = 12,
  ar_dat = 13,

  mt_type = 20,
  mt_numy = 21,
  mt_numx = 22,
  mt_dat_len = 23,
  mt_dat = 24,

  st_fs_len = 30,
  st_fs = 31,
  st_dat_len = 32,
  st_dat = 33
};

inline STATE& operator++(STATE& f, int) {
  switch (f) {
    case STATE::HEADER:
      return f = STATE::pkg_len;
    case STATE::pkg_len:
      return f = STATE::switch_type;
    case STATE::ar_type:
      return f = STATE::ar_num;
    case STATE::ar_num:
      return f = STATE::ar_dat_len;
    case STATE::ar_dat_len:
      return f = STATE::ar_dat;
    case STATE::ar_dat:
      return f = STATE::chksum;
    case STATE::mt_type:
      return f = STATE::mt_numy;
    case STATE::mt_numy:
      return f = STATE::mt_numx;
    case STATE::mt_numx:
      return f = STATE::mt_dat_len;
    case STATE::mt_dat_len:
      return f = STATE::mt_dat;
    case STATE::mt_dat:
      return f = STATE::chksum;
    case STATE::st_fs_len:
      return f = STATE::st_fs;
    case STATE::st_fs:
      return f = STATE::st_dat_len;
    case STATE::st_dat_len:
      return f = STATE::st_dat;
    case STATE::st_dat:
      return f = STATE::chksum;
    case STATE::chksum:
      return f = STATE::HEADER;
    default:
      return f;
  }
}

struct HMI_format {
 public:
  PAC_type type;
  ssub_match format;
  HMI_format(PAC_type type, ssub_match format) : type(type), format(format) {}
};

/**
 * @brief from serial port to PC
 * 
 */
class ASADecode {
 private:
  // member
  STATE decodeState = STATE::HEADER;
  uint16_t count = 0;
  uint16_t paclen = 0;
  uint8_t chksum = 0;
  uint8_t pkg_type = 0;

  uint8_t ar_type = 0;
  uint8_t ar_num = 0;
  uint16_t ar_dlen = 0;
  vector<uint8_t> ar_dat;

  uint8_t mt_type = 0;
  uint8_t mt_numy = 0;
  uint8_t mt_numx = 0;
  uint16_t mt_dlen = 0;
  vector<uint8_t> mt_dat;

  uint8_t st_fs_len = 0;
  vector<uint8_t> st_fs;
  uint16_t st_dlen = 0;
  vector<uint8_t> st_dat;

  // function
  void clear();
  template <typename T>
  string transfirm(vector<uint8_t> data);
  inline string dataTransfirm(HMI_type type, vector<uint8_t> data);
  

 public:
  // member
  bool isProcessing = false;
  bool isDone = false;

  // function
  ASADecode();
  ~ASADecode();

  bool put(uint8_t buff);
  string get();
  void putArray(uint8_t ar_type,uint8_t ar_num);
  void putMatrix(uint8_t mt_type,uint8_t mt_numy,uint8_t mt_numx);
  void putStruct(string st_fs);
  static bool isSync(char buff);
  inline string getTypeStr(int typeNum)
  {
  if (typeNum == 0)
    return "i8"s;
  else if (typeNum == 1)
    return "i16"s;
  else if (typeNum == 2)
    return "i32"s;
  else if (typeNum == 3)
    return "i64"s;
  else if (typeNum == 4)
    return "ui8"s;
  else if (typeNum == 5)
    return "ui16"s;
  else if (typeNum == 6)
    return "ui32"s;
  else if (typeNum == 7)
    return "ui64"s;
  else if (typeNum == 8)
    return "f32"s;
  else if (typeNum == 9)
    return "f64"s;
  else if (typeNum == 15)
    return "s"s;
  else
    return ""s;
}
};

/**
 * @brief from PC to serial port
 * 
 */
class ASAEncode {
  struct SplitStr{
    string str;
    int64_t lastIndex;
  };

 private:
  // member
  bool isError = false;

  static constexpr array<uint8_t, 3> _HEADER = {0xac, 0xac, 0xac};
  PAC_type pkg_type;

  uint8_t ar_type = 0;
  uint8_t ar_num = 0;
  vector<uint8_t> ar_dat;

  uint8_t mt_type = 0;
  uint8_t mt_numy = 0;
  uint8_t mt_numx = 0;
  vector<uint8_t> mt_dat;

  // uint16_t st_fs_len = 0;
  vector<uint8_t> st_fs;
  vector<uint8_t> st_dat;

  vector<uint8_t> dat;
  regex
      ar_re;  //{"[if](?:8|16|32|64)_[0-9]+\\s*:\\s*{)+(?:[^{}]*)}"s,regex::optimize
              //};
  regex
      mt_re;  //{"[if](?:8|16|32|64)_[0-9]+(?:x[0-9]+)\\s*:(\\s*{)(?:\\s*{[^{}]*}\\s*)"
              //"}"s,regex::optimize };
  regex
      st_re;  //{"[if](?:8|16|32|64)_[0-9]+\\s*(?:,\\s*[if](?:8|16|32|64)_[0-9]+\\s*)+"
              // ":(\\s*{)(?:\\s*[if](?:8|16|32|64)_[0-9]+\\s*:\\s*{[^{}]*}\\s*)+}"s,
              // regex::optimize };
  // function
  // inline uint8_t getTypeNum(string typeStr);
  template <typename T, typename f>
  vector<uint8_t> transfirm(string data, const char* format, uint8_t chklen,
                            f h2be);
  vector<uint8_t> transfirm2data(HMI_type type, string data, uint8_t chklen);

  vector<uint8_t> encodeAr2Pac();
  vector<uint8_t> encodeMt2Pac();
  vector<uint8_t> encodeSt2Pac();

 public:
 
  // member
  // function
  ASAEncode();
  ~ASAEncode();

  static bool isSync(char buff);
  static vector<SplitStr> split(string text);
  //   bool put(HMI_format format);
  bool put(string text);
  vector<uint8_t> get();
  void clear();

  
};

}  // namespace ASAEncoder