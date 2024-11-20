#include "asaFile.h"
#include "asaFormat.h"
namespace ASAEncoder {

struct HMI_format {
 public:
  PAC_type type;
  ssub_match format;
  HMI_format(PAC_type type, ssub_match format) : type(type), format(format) {}
};

class ASABasic {
 protected:
  uint8_t pkg_type = 0;

  uint8_t ar_type = 0;
  uint8_t ar_num = 0;
  vector<uint8_t> ar_dat;

  uint8_t mt_type = 0;
  uint8_t mt_numy = 0;
  uint8_t mt_numx = 0;
  vector<uint8_t> mt_dat;

  vector<uint8_t> st_fs;
  vector<uint8_t> st_dat;

  template <typename T>
  string transfirm(vector<uint8_t> data);
  inline string dataTransfirm(HMI_type type, vector<uint8_t> data);

 public:
  ASABasic();
  virtual ~ASABasic();
  static inline string type2str(int typeNum) {
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
  static inline string pacTypeStr(int typeNum) {
    if (typeNum == 1)
      return "AR"s;
    else if (typeNum == 2)
      return "MT"s;
    else if (typeNum == 3)
      return "ST"s;
    else
      return ""s;
  }
  int getType();
  string getFormat();
  string detailStr();
};
/**
 * @brief from serial port to PC
 *
 */
class ASADecode : public ASABasic {
 private:
  // member
  STATE decodeState = STATE::HEADER;
  uint16_t count = 0;
  uint16_t paclen = 0;
  uint8_t chksum = 0;
  ASAFileDec fdec;
  // uint16_t ar_dlen = 0;

  // uint16_t mt_dlen = 0;

  // uint8_t st_fs_len = 0;
  // uint16_t st_dlen = 0;

  // function
  void clear();

 public:
  // member
  static bool sync;
  bool isProcessing = false;
  bool isDone = false;
  bool isFile = false;
  bool retry = false;
  // function
  ASADecode();
  ~ASADecode();

  bool put(uint8_t buff);
  string get();
  void putArray(uint8_t ar_type, uint8_t ar_num);
  void putMatrix(uint8_t mt_type, uint8_t mt_numy, uint8_t mt_numx);
  void putStruct(string st_fs);
  static bool isSync(char buff);
};

/**
 * @brief from PC to serial port
 *
 */
class ASAEncode : public ASABasic {
  struct SplitStr {
    string str;
    int64_t lastIndex;
  };

 private:
  // member
  bool isError = false;
  bool isFile = false;
  ASAFileEnc fEnc;
  static constexpr array<uint8_t, 3> _HEADER = {0xac, 0xac, 0xac};
  // PAC_type pkg_type;

  // uint8_t ar_type = 0;
  // uint8_t ar_num = 0;
  // vector<uint8_t> ar_dat;

  // uint8_t mt_type = 0;
  // uint8_t mt_numy = 0;
  // uint8_t mt_numx = 0;
  // vector<uint8_t> mt_dat;

  // // uint16_t st_fs_len = 0;
  // vector<uint8_t> st_fs;
  // vector<uint8_t> st_dat;

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
  vector<uint8_t> encodeF2Pac(bool next = true);

 public:
  // member
  // function
  ASAEncode();
  ~ASAEncode();

  static bool isSync(char buff);
  static vector<SplitStr> split(string text);
  //   bool put(HMI_format format);
  bool put(string text);
  bool putFile(string fileName);
  vector<uint8_t> get(bool next = true);
  void clear();
};

}  // namespace ASAEncoder