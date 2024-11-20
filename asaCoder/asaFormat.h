#ifndef ASAFORMAT_H
#define ASAFORMAT_H
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

enum PAC_type : uint8_t { AR = 1, MT = 2, ST = 3, F=4 };

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
  st_dat = 33,

  file = 40

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
    case STATE::file:
      return f = STATE::chksum;
    case STATE::chksum:
      return f = STATE::HEADER;
    default:
      return f;
  }
}

#define FILE_VERSION 1
}  // namespace ASAEncoder

#endif // ASAFORMAT_H