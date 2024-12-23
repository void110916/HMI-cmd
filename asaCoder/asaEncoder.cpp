#include "asaEncoder.h"

// #include <QDebug>
// #include <QString>
#include <cstdint>
#include <cstdio>
#include <format>
#include <limits>

#include "endianness.h"

namespace ASAEncoder {
// ASABasic
ASABasic::ASABasic() {}

ASABasic::~ASABasic() {}

string ASABasic::getFormat() {
  string str;
  if (pkg_type == PAC_type::AR) {
    str = std::format("{:s}_{:d}", type2str(ar_type), ar_num);
  } else if (pkg_type == PAC_type::MT) {
    str = std::format("{:s}_{:d}x{:d}", type2str(mt_type), mt_numy, mt_numx);
  } else if (pkg_type == PAC_type::ST) {
    str = string(reinterpret_cast<char*>(st_fs.data()), st_fs.size());
  }
  return str;
}

string ASABasic::detailStr() {
  string text = "";
  if (pkg_type == PAC_type::AR) {
    text = getFormat() + " :\n    {{ " +
           dataTransfirm((HMI_type)ar_type, ar_dat) + " }}\n\n";
  } else if (pkg_type == PAC_type::MT) {
    string mt;
    int mt_sizeof = mt_dat.size() / (mt_numy * mt_numx);
    for (int i = 0; i < mt_numy; i++) {
      auto&& it = mt_dat.begin() + mt_sizeof * mt_numx * i;
      vector<uint8_t> oneline(make_move_iterator(it),
                              make_move_iterator(it + mt_numx * mt_sizeof));
      string&& st = dataTransfirm((HMI_type)mt_type, oneline);
      mt += "    { "s + st + " }\n"s;
    }
    text = getFormat() + " :\n{\n" + mt + "}\n\n";
  } else if (pkg_type == PAC_type::ST) {
    text = getFormat();
    auto type = std::istringstream(text);
    text = regex_replace(text, regex(","), " , ") + " :\n{\n";
    string d;
    while (std::getline(type, d, ',')) {
      array<string, 2> info;
      auto&& at = d.find("_");
      info[0] = d.substr(0, at);
      info[1] = d.substr(at + 1);
      vector<uint8_t> dat;
      auto&& it = make_move_iterator(st_dat.begin());
      string st;
      if (info[0] == "ui8"s) {
        dat.insert(dat.begin(), it,
                   it + std::stoi(info[1]) * sizeof(std::uint8_t));
        st = dataTransfirm(HMI_type::UI8, dat);
      } else if (info[0] == "ui16"s) {
        dat.insert(dat.begin(), it,
                   it + std::stoi(info[1]) * sizeof(std::uint16_t));
        st = dataTransfirm(HMI_type::UI16, dat);
      } else if (info[0] == "ui32"s) {
        dat.insert(dat.begin(), it,
                   it + std::stoi(info[1]) * sizeof(std::uint32_t));
        st = dataTransfirm(HMI_type::UI32, dat);
      } else if (info[0] == "ui64"s) {
        dat.insert(dat.begin(), it,
                   it + std::stoi(info[1]) * sizeof(std::uint64_t));
        st = dataTransfirm(HMI_type::UI64, dat);
      } else if (info[0] == "i8"s) {
        dat.insert(dat.begin(), it,
                   it + std::stoi(info[1]) * sizeof(std::int8_t));
        st = dataTransfirm(HMI_type::I8, dat);
      } else if (info[0] == "i16"s) {
        dat.insert(dat.begin(), it,
                   it + std::stoi(info[1]) * sizeof(std::int16_t));
        st = dataTransfirm(HMI_type::I16, dat);
      } else if (info[0] == "i32"s) {
        dat.insert(dat.begin(), it,
                   it + std::stoi(info[1]) * sizeof(std::int32_t));
        st = dataTransfirm(HMI_type::I32, dat);
      } else if (info[0] == "i64"s) {
        dat.insert(dat.begin(), it,
                   it + std::stoi(info[1]) * sizeof(std::int64_t));
        st = dataTransfirm(HMI_type::I64, dat);
      } else if (info[0] == "f32"s) {
        dat.insert(dat.begin(), it, it + std::stoi(info[1]) * sizeof(float));
        st = dataTransfirm(HMI_type::F32, dat);
      } else if (info[0] == "f64"s) {
        dat.insert(dat.begin(), it, it + std::stoi(info[1]) * sizeof(double));
        st = dataTransfirm(HMI_type::F64, dat);
      }
      text += "    :{ "s + st + " }\n";
    }
    text += "}\n\n"s;
  }
  return text;
}

int ASABasic::getType() { return pkg_type; }

// ASADecode
bool ASADecode::sync = false;
ASADecode::ASADecode() { sync = false; }

ASADecode::~ASADecode() {}

bool ASADecode::isSync(char buff) {
  static string cmd;
  static bool inSync = false;
  static regex re(R"(^~G[AMSF])");
  if (buff == '~') inSync = true;
  if (inSync) cmd += buff;
  if (inSync && buff == '\n') {
    if (regex_search(cmd, re)) {
      inSync = false;
      sync = true;
      cmd.clear();
      return true;
    }
    cmd.clear();
  }
  return false;
}

bool ASADecode::put(uint8_t buff) {
  static uint16_t dlen = 0;
  if (isProcessing) {
    switch (decodeState) {
      case STATE::HEADER:
        if (buff == 0xac) {
          count++;
          if (count == 3) {
            decodeState++;
            count = 0;
          }
        } else {
          isProcessing = false;
          return false;
        }
        break;
      case STATE::pkg_len:
        if (count == 0) {
          paclen = buff << 8;
          count++;
        } else {
          paclen += buff & UINT8_MAX;
          count = 0;
          decodeState++;
        }
        break;
      case STATE::switch_type:
        chksum += buff;
        pkg_type = buff;
        if (buff == 1)
          decodeState = STATE::ar_type;
        else if (buff == 2)
          decodeState = STATE::mt_type;
        else if (buff == 3)
          decodeState = STATE::st_fs_len;
        else if (buff == 4)
          decodeState = STATE::file;
        break;
      case STATE::ar_type:
        chksum += buff;
        ar_type = buff;
        decodeState++;
        break;
      case STATE::ar_num:
        chksum += buff;
        ar_num = buff;
        decodeState++;
        break;
      case STATE::ar_dat_len:
        chksum += buff;
        if (count == 0) {
          dlen = buff << 8;
          count = 1;
        } else {
          dlen += buff;
          count = 0;
          ar_dat.resize(dlen);
          decodeState++;
        }
        break;
      case STATE::ar_dat:
        chksum += buff;
        ar_dat[count++] = buff;
        if (count == dlen) {
          count = 0;
          decodeState++;
        }
        break;
      case STATE::mt_type:
        chksum += buff;
        mt_type = buff;
        decodeState++;
        break;
      case STATE::mt_numy:
        chksum += buff;
        mt_numy = buff;
        decodeState++;
        break;
      case STATE::mt_numx:
        chksum += buff;
        mt_numx = buff;
        decodeState++;
        break;
      case STATE::mt_dat_len:
        chksum += buff;
        if (count == 0) {
          dlen = buff << 8;
          count = 1;
        } else {
          dlen += buff;
          count = 0;
          mt_dat.resize(dlen);
          decodeState++;
        }
        break;
      case STATE::mt_dat:
        chksum += buff;
        mt_dat[count++] = buff;
        if (count == dlen) {
          count = 0;
          decodeState++;
        }
        break;
      case STATE::st_fs_len:
        chksum += buff;
        dlen = buff;
        st_fs.resize(dlen);
        decodeState++;
        break;
      case STATE::st_fs:
        chksum += buff;
        st_fs[count++] = buff;
        if (count == dlen) {
          count = 0;
          decodeState++;
        }
        break;
      case STATE::st_dat_len:
        chksum += buff;
        if (count == 0) {
          dlen = buff << 8;
          count = 1;
        } else {
          count = 0;
          dlen += buff;
          st_dat.resize(dlen);
          decodeState++;
        }
        break;
      case STATE::st_dat:
        chksum += buff;
        st_dat[count++] = buff;
        if (count == dlen) {
          count = 0;
          decodeState++;
        }
        break;
      case STATE::file:
        chksum += buff;
        if (++count == paclen - 2) {
          count = 0;
          decodeState++;
        }
        if (!fdec.put(buff)) {
          retry = true;
          return false;
        }
        break;
      case STATE::chksum:
        decodeState++;
        isProcessing = false;
        if ((chksum & UINT8_MAX) == buff) {
          retry = false;
          if (pkg_type == F) {
            isFile = true;
            if (fdec.save()) isDone = true;
          } else
            isDone = true;
        } else {
          retry = true;
        }
        break;
    }
  } else {
    if (buff == 0xac) {
      clear();
      dlen = 0;
      isProcessing = true;
      count++;
    } else
      return false;
  }
  return true;
}

string ASADecode::get() {
  // if (!isDone) return "";
  string text = "";
  if (pkg_type == PAC_type::AR) {
    text = getFormat() + " :\n    {{ " +
           dataTransfirm((HMI_type)ar_type, ar_dat) + " }}\n";
  } else if (pkg_type == PAC_type::MT) {
    string mt;
    int mt_sizeof = mt_dat.size() / (mt_numy * mt_numx);
    for (int i = 0; i < mt_numy; i++) {
      auto&& it = mt_dat.begin() + mt_sizeof * mt_numx * i;
      vector<uint8_t> oneline(make_move_iterator(it),
                              make_move_iterator(it + mt_numx * mt_sizeof));
      string&& st = dataTransfirm((HMI_type)mt_type, oneline);
      mt += "    { "s + st + " }\n"s;
    }
    text = getFormat() + " :\n{\n" + mt + "}\n";
  } else if (pkg_type == PAC_type::ST) {
    text = getFormat();
    auto type = std::istringstream(text);
    text = regex_replace(text, regex(","), " , ") + " :\n{\n";
    string d;
    auto&& it = make_move_iterator(st_dat.begin());
    while (std::getline(type, d, ',')) {
      array<string, 2> info;
      auto&& at = d.find("_");
      info[0] = d.substr(0, at);
      info[1] = d.substr(at + 1);
      vector<uint8_t> dat;

      string st;
      HMI_type HMItype;
      size_t len;
      if (info[0] == "ui8"s) {
        len = std::stoi(info[1]) * sizeof(uint8_t);
        HMItype = HMI_type::UI8;
      } else if (info[0] == "ui16"s) {
        len = std::stoi(info[1]) * sizeof(uint16_t);
        HMItype = HMI_type::UI16;
      } else if (info[0] == "ui32"s) {
        len = std::stoi(info[1]) * sizeof(uint32_t);
        HMItype = HMI_type::UI32;
      } else if (info[0] == "ui64"s) {
        len = std::stoi(info[1]) * sizeof(uint64_t);
        HMItype = HMI_type::UI64;
      } else if (info[0] == "i8"s) {
        len = std::stoi(info[1]) * sizeof(int8_t);
        HMItype = HMI_type::I8;
      } else if (info[0] == "i16"s) {
        len = std::stoi(info[1]) * sizeof(int16_t);
        HMItype = HMI_type::I16;
      } else if (info[0] == "i32"s) {
        len = std::stoi(info[1]) * sizeof(int32_t);
        HMItype = HMI_type::I32;
      } else if (info[0] == "i64"s) {
        len = std::stoi(info[1]) * sizeof(int64_t);
        HMItype = HMI_type::I64;
      } else if (info[0] == "f32"s) {
        len = std::stoi(info[1]) * sizeof(float);
        HMItype = HMI_type::F32;
      } else if (info[0] == "f64"s) {
        len = std::stoi(info[1]) * sizeof(double);
        HMItype = HMI_type::F64;
      }
      dat.insert(dat.begin(), it, it + len);
      st = dataTransfirm(HMItype, dat);
      it += len;
      text += "    :{ "s + st + " }\n";
    }
    text += "}\n"s;
  } else if (pkg_type == PAC_type::F) {
    
  }
  // clear();
  isDone = false;
  isFile = false;
  sync = false;
  return text;
}

void ASADecode::putArray(uint8_t ar_type, uint8_t ar_num) {
  const uint8_t typeSize[] = {1, 2, 4, 8, 1, 2, 4, 8, 4, 8};
  this->pkg_type = PAC_type::AR;
  this->ar_type = ar_type;
  this->ar_num = ar_num;
  ar_dat.resize(ar_num * typeSize[ar_type]);
}

void ASADecode::putMatrix(uint8_t mt_type, uint8_t mt_numy, uint8_t mt_numx) {
  const uint8_t typeSize[] = {1, 2, 4, 8, 1, 2, 4, 8, 4, 8};
  this->pkg_type = PAC_type::MT;
  this->mt_type = mt_type;
  this->mt_numy = mt_numy;
  this->mt_numx = mt_numx;
  mt_dat.resize(mt_numy * mt_numx * typeSize[mt_type]);
}

void ASADecode::putStruct(string st_fs) {
  this->pkg_type = PAC_type::ST;
  this->st_fs.insert(this->st_fs.begin(), std::move_iterator(st_fs.begin()),
                     std::move_iterator(st_fs.end()));
  // this->st_fs_len = this->st_fs.size();
  this->st_dat.resize(UINT16_MAX);
}

void ASADecode::clear() {
  isProcessing = false;
  decodeState = STATE::HEADER;
  count = 0;
  paclen = 0;

  chksum = 0;
  pkg_type = 0;

  ar_type = 0;
  ar_num = 0;
  // ar_dlen = 0;
  ar_dat.clear();

  mt_type = 0;
  mt_numy = 0;
  mt_numx = 0;
  // mt_dlen = 0;
  mt_dat.clear();

  // st_fs_len = 0;
  st_fs.clear();
  // st_dlen = 0;
  st_dat.clear();

  // isDone = false;
}

template <typename T>
string ASABasic::transfirm(vector<uint8_t> data) {
  string o;
  T* ptr = reinterpret_cast<T*>(data.data());
  for (ptr; ptr < reinterpret_cast<T*>(&(*data.end())); ptr++)
    o += std::to_string(*ptr) + ", "s;
  return o.substr(0, o.length() - 2);
}

inline string ASABasic::dataTransfirm(HMI_type type, vector<uint8_t> data) {
  if (type == HMI_type::I8)
    return transfirm<int8_t>(data);
  else if (type == HMI_type::I16)
    return transfirm<int16_t>(data);
  else if (type == HMI_type::I32)
    return transfirm<int32_t>(data);
  else if (type == HMI_type::I64)
    return transfirm<int64_t>(data);
  else if (type == HMI_type::UI8)
    return transfirm<uint8_t>(data);
  else if (type == HMI_type::UI16)
    return transfirm<uint16_t>(data);
  else if (type == HMI_type::UI32)
    return transfirm<uint32_t>(data);
  else if (type == HMI_type::UI64)
    return transfirm<uint64_t>(data);
  else if (type == HMI_type::F32)
    return transfirm<float>(data);
  else if (type == HMI_type::F64)
    return transfirm<double>(data);
  else
    return "";
}

// ASAEncode
ASAEncode::ASAEncode() {
  ar_re = regex("[if](?:8|16|32|64)_[0-9]+\\s*:\\s*\\{+(?:[^\\{\\}]*)\\}",
                regex::ECMAScript | regex::optimize);
  mt_re = regex(
      "[if](?:8|16|32|64)_[0-9]+(?:x[0-9]+)\\s*:(\\s*\\{)(?:\\s*\\{[^\\{\\}]*"
      "\\}\\s*)\\}",
      regex::ECMAScript | regex::optimize);
  st_re = regex(
      "[if](?:8|16|32|64)_[0-9]+\\s*(?:,\\s*[if](?:8|16|32|64)_[0-9]+\\s*)+:("
      "\\s*\\{)(?:\\s*[if](?:8|16|32|64)_[0-9]+\\s*:\\s*\\{[^\\{\\}]*\\}\\s*)+"
      "\\}",
      regex::ECMAScript | regex::optimize);
}

ASAEncode::~ASAEncode() {}

bool ASAEncode::isSync(char buff) {
  static string cmd;
  static bool inSync = false;
  static regex re(R"(^~P[AMSF])");
  if (buff == '~') inSync = true;
  if (inSync) cmd += buff;
  if (inSync && buff == '\n') {
    if (regex_search(cmd, re)) {
      inSync = false;
      cmd.clear();
      return true;
    }
    cmd.clear();
  }
  return false;
}

static inline uint8_t getTypeNum(string typeStr) {
  // auto s=typeStr.find("i8"s)==string::npos;
  if (typeStr.find("i8"s) != string::npos)
    return 0;
  else if (typeStr.find("i16"s) != string::npos)
    return 1;
  else if (typeStr.find("i32"s) != string::npos)
    return 2;
  else if (typeStr.find("i64"s) != string::npos)
    return 3;
  else if (typeStr.find("ui8"s) != string::npos)
    return 4;
  else if (typeStr.find("ui16"s) != string::npos)
    return 5;
  else if (typeStr.find("ui32"s) != string::npos)
    return 6;
  else if (typeStr.find("ui64"s) != string::npos)
    return 7;
  else if (typeStr.find("f32"s) != string::npos)
    return 8;
  else if (typeStr.find("f64"s) != string::npos)
    return 9;
  else if (typeStr.find("s"s) != string::npos)
    return 15;
  else
    return ~0;
}

vector<uint8_t> ASAEncode::encodeAr2Pac() {
  /**
   * header (3)
   * pac_len (2) = 1 + 1 + 1 + 2 + N = N + 5
   * pac_type (1)
   * data_type (1)
   * data_num (1)
   * data_len (2) (data_bytes)
   * data (N)
   * chknum (1) (pac_type + data_type + data_len + data)
   */

  vector<uint8_t> pac(_HEADER.begin(), _HEADER.end());
  pac.reserve(11 + dat.size());
  vector<uint8_t> payload;
  payload.reserve(5 + dat.size());
  uint16_t&& blen = dat.size();

  payload.push_back(PAC_type::AR);
  payload.push_back(ar_type);
  payload.push_back(ar_num);
  payload.push_back(blen >> 8);
  payload.push_back(blen & UINT8_MAX);

  payload.insert(payload.end(), dat.begin(), dat.end());

  uint8_t sum = 0;
  for (auto&& it : payload) sum += it;
  uint16_t&& size = payload.size();
  pac.push_back(size >> 8);
  pac.push_back(size & UINT8_MAX);
  pac.insert(pac.end(), make_move_iterator(payload.begin()),
             make_move_iterator(payload.end()));
  pac.push_back(sum);
  return pac;
}

vector<uint8_t> ASAEncode::encodeMt2Pac() {
  /**
   * header (3)
   * pac_len (2) = 1 + 1 + 1 + 1 + 2 + N = N + 6
   * pac_type (1)
   * data_type (1)
   * data_dim1 (1)
   * data_dim2 (1)
   * data_len (2) (data_bytes)
   * data (N)
   * chknum (1) (pac_type + data_type + data_len + data)
   */

  vector<uint8_t> pac(_HEADER.begin(), _HEADER.end());
  pac.reserve(12 + dat.size());

  vector<uint8_t> payload;
  payload.reserve(6 + dat.size());
  payload.push_back(PAC_type::MT);
  payload.push_back(mt_type);
  payload.push_back(mt_numy);
  payload.push_back(mt_numx);
  uint16_t&& blen = dat.size();
  payload.push_back(blen >> 8);
  payload.push_back(blen & UINT8_MAX);

  payload.insert(payload.end(), dat.begin(), dat.end());

  uint8_t sum = 0;
  for (auto&& it : payload) sum += it;
  uint16_t&& paclen = payload.size();
  pac.push_back(paclen >> 8);
  pac.push_back(paclen & UINT8_MAX);
  pac.insert(pac.end(), make_move_iterator(payload.begin()),
             make_move_iterator(payload.end()));
  pac.push_back(sum);
  return pac;
}

vector<uint8_t> ASAEncode::encodeSt2Pac() {
  /**
   * header (3)
   * pac_len (2) = 1 + 1 + M + 2 + N = N + M + 4
   * pac_type (1)
   * fs_len (1)
   * fs (M)
   * data_len (2) (data_bytes)
   * data (N)
   * chknum (1) (pac_type + fs_len + fs + data_len + data)
   */
  vector<uint8_t> pac(_HEADER.begin(), _HEADER.end());
  pac.reserve(10 + st_fs.size() + st_dat.size());
  vector<uint8_t> payload;
  payload.reserve(st_fs.size() + dat.size() + 5);
  payload.push_back(PAC_type::ST);
  payload.push_back(st_fs.size());
  payload.insert(payload.end(), make_move_iterator(st_fs.begin()),
                 make_move_iterator(st_fs.end()));
  uint16_t&& blen = dat.size();
  payload.push_back(blen >> 8);
  payload.push_back(blen & UINT8_MAX);

  payload.insert(payload.end(), dat.begin(), dat.end());

  uint8_t sum = 0;
  for (auto&& it : payload) sum += it;
  uint16_t&& paclen = payload.size();
  pac.push_back(paclen >> 8);
  pac.push_back(paclen & UINT8_MAX);
  pac.insert(pac.end(), make_move_iterator(payload.begin()),
             make_move_iterator(payload.end()));
  pac.push_back(sum);
  return pac;
}

vector<uint8_t> ASAEncode::encodeF2Pac(bool next) {
  /**
   * header (3)
   * pac_len (2) = 1 + N
   * pac_type (1)
   * data (N)
   * chknum (1) (pac_type + data)
   */
  vector<uint8_t> pac(_HEADER.begin(), _HEADER.end());
  vector<uint8_t> payload = fEnc.get(next);
  if (payload.empty()) return payload;
  uint8_t sum = PAC_type::F;
  for (auto&& it : payload) sum += it;
  uint16_t&& paclen = payload.size();
  pac.reserve(7 + paclen);
  pac.push_back(paclen >> 8);
  pac.push_back(paclen & UINT8_MAX);
  pac.push_back(PAC_type::F);
  pac.insert(pac.end(), make_move_iterator(payload.begin()),
             make_move_iterator(payload.end()));
  pac.push_back(sum);
  return pac;
}

vector<ASAEncode::SplitStr> ASAEncode::split(string text) {
  static regex re(
      "[if](?:8|16|32|64)_[0-9]+(?:x[0-9]+)?\\s*(?:,\\s*[if](?:8|16|32|64)_[0-"
      "9]+\\s*)*:\\s*\\{\\s*(?:\\{[^\\{\\}]+\\}\\s*|[^\\{\\}]+\\s*)+\\}",
      regex::ECMAScript | regex::optimize);
  vector<ASAEncode::SplitStr> ret;
  smatch matchs;
  int pos = 0;
  //, std::regex_constants::match_any
  while (std::regex_search(
      text, matchs, re,
      std::regex_constants::match_any | std::regex_constants::match_not_null)) {
    // if (match.matched)

    ret.push_back(ASAEncode::SplitStr{
        matchs.str(), matchs.position() + matchs.length() + pos});
    pos = matchs.length();
    text = matchs.suffix();
  }
  return ret;
}

bool ASAEncode::put(string text) {
  static regex re("\\{([^\\{\\}]+)\\}", regex::ECMAScript | regex::optimize);
  static regex type(
      "[if](?:8|16|32|64)_[0-9]+(?:x[0-9]+)?\\s*(?:,\\s*[if](?:8|16|32|64)_[0-"
      "9]+\\s*)*",
      regex::ECMAScript | regex::optimize);
  static regex space("\\s");
  smatch matchs;
  text = std::regex_replace(text, space, ""s);
  // int pos = text.find(":");
  // if (pos == string::npos) return false;
  std::regex_search(text, matchs, type);
  string&& rstring = matchs[0].str();
  string typeStr = rstring.substr(0, rstring.find('_'));
  string length = rstring.substr(rstring.find('_') + 1);
  size_t pos = length.find('x');
  if (pos != string::npos) {
    pkg_type = PAC_type::MT;
    mt_type = getTypeNum(typeStr);
    char* d = length.substr(pos + 1).data();
    mt_numy = strtol(length.substr(0, pos).data(), NULL, 10);
    mt_numx = strtol(length.substr(pos + 1).data(), NULL, 10);
    // std::sscanf(length.substr(pos + 1).data(), "%d", &mt_numx);
    // std::sscanf(length.substr(0, pos).data(), "%d", &mt_numy);
    uint8_t countY = 0;
    while (std::regex_search(text, matchs, re)) {
      countY++;
      auto tmp = transfirm2data((HMI_type)mt_type, matchs[1].str(), mt_numx);
      if (tmp.empty()) {
        clear();
        return false;
      }
      dat.insert(dat.end(), tmp.begin(), tmp.end());
      text = matchs.suffix();
    }
    if (countY != mt_numy) {
      clear();
      return false;
    }
  } else {
    pos = rstring.find(",");
    size_t ppos = string::npos;
    if (pos != string::npos) {
      pkg_type = PAC_type::ST;
      st_fs.insert(st_fs.end(), rstring.begin(), rstring.end());
    } else {
      pkg_type = PAC_type::AR;
      while (std::regex_search(text, matchs, re)) {
        ar_type = getTypeNum(rstring.substr(ppos + 1, pos));
        std::sscanf(
            rstring.substr(rstring.find('_', ppos + 1) + 1, pos).c_str(), "%d",
            &ar_num);
        ppos = pos;
        auto tmp = transfirm2data((HMI_type)ar_type, matchs[1].str(), ar_num);
        if (tmp.empty()) {
          clear();
          return false;
        }
        dat.insert(dat.end(), tmp.begin(), tmp.end());
        text = matchs.suffix();
      }
    }
  }
  return true;
}

bool ASAEncode::putFile(string fileName) {
  if (!fEnc.put(fileName)) return false;
  isFile = true;
  pkg_type = PAC_type::F;
  return true;
}

template <typename T, typename f>
vector<uint8_t> ASAEncode::transfirm(string data, const char* format,
                                     uint8_t chklen, f h2be) {
  vector<uint8_t> raw;
  vector<size_t> numStr;
  size_t pos = string::npos;
  do {
    pos = data.find(',', pos + 1);
    numStr.push_back(pos);
    // pos = data.find(',', pos + 1);
  } while (pos != string::npos);

  auto len = numStr.size();
  raw.resize(len * sizeof(T));
  auto ptr = reinterpret_cast<T*>(raw.data());
  for (int i = 0, ppos = 0; i < len; i++) {
    T num;
    auto s = data.substr(ppos, numStr[i]).c_str();
    std::sscanf(s, format, &num);
    *(ptr + i) = h2be(num);
    ppos = numStr[i] + 1;
  }
  return raw;
}

vector<uint8_t> ASAEncode::transfirm2data(HMI_type type, string data,
                                          uint8_t chklen) {
  if (type == HMI_type::I8)
    return transfirm<int8_t>(data, "%d", chklen, [](int8_t x) { return x; });
  else if (type == HMI_type::I16)
    return transfirm<int16_t>(data, "%d", chklen,
                              [](int16_t x) { return htole16(x); });
  else if (type == HMI_type::I32)
    return transfirm<int32_t>(data, "%d", chklen,
                              [](int32_t x) { return htole32(x); });
  else if (type == HMI_type::I64)
    return transfirm<int64_t>(data, "%ld", chklen,
                              [](int32_t x) { return htole64(x); });
  else if (type == HMI_type::UI8)
    return transfirm<uint8_t>(data, "%u", chklen, [](uint8_t x) { return x; });
  else if (type == HMI_type::UI16)
    return transfirm<uint16_t>(data, "%u", chklen,
                               [](int16_t x) { return htole16(x); });
  else if (type == HMI_type::UI32)
    return transfirm<uint32_t>(data, "%u", chklen,
                               [](int32_t x) { return htole32(x); });
  else if (type == HMI_type::UI64)
    return transfirm<uint64_t>(data, "%lu", chklen,
                               [](int32_t x) { return htole64(x); });
  else if (type == HMI_type::F32)
    return transfirm<float>(data, "%f", chklen,
                            [](float x) { return htole32(x); });
  else if (type == HMI_type::F64)
    return transfirm<double>(data, "%lf", chklen,
                             [](double x) { return htole64(x); });
  else
    return vector<uint8_t>();
}
vector<uint8_t> ASAEncode::get(bool next) {
  if (pkg_type == PAC_type::AR)
    return encodeAr2Pac();
  else if (pkg_type == PAC_type::MT)
    return encodeMt2Pac();
  else if (pkg_type == PAC_type::ST)
    return encodeSt2Pac();
  else if (pkg_type == PAC_type::F)
    return encodeF2Pac(next);
  return vector<uint8_t>();
}

void ASAEncode::clear() {
  isError = false;

  // PAC_type pkg_type = std::nullopt;

  ar_type = 0;
  ar_num = 0;
  ar_dat.clear();

  mt_type = 0;
  mt_numy = 0;
  mt_numx = 0;
  mt_dat.clear();

  // st_fs_len = 0;
  st_fs.clear();
  st_dat.clear();
}
}  // namespace ASAEncoder
