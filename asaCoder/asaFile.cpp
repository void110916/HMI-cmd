#include "asaFile.h"

#include <filesystem>
#include <fstream>
namespace ASAEncoder {
using namespace std;
namespace fs = filesystem;
// ASA File Basic
ASAFileBasic::ASAFileBasic() {}

ASAFileBasic::~ASAFileBasic() {}
// ASA File Encode
ASAFileEnc::ASAFileEnc() {}

ASAFileEnc::~ASAFileEnc() {}
bool ASAFileEnc::put(string fileName) {
  if (!fs::exists(fileName)) return false;
  if (fileName.size() > 7) false;
  if (fs::path(fileName).extension() == ".txt") ftype = ASCII;

  std::fstream f;
  f.open(fileName, std::ios::in);
  do {
    FPAC fpac;
    fpac.pac.data[0] = idx++;
    f.read(reinterpret_cast<char *>(fpac.pac.data.data() + 1), 256);
    // int p = f.gcount();
    // if (p != 256) {
    //   fill(fpac.pac.data.begin() + p+1, fpac.pac.data.end(), 0);
    // }
    // for (auto&& it : fpac.pac.data) {
    //   fpac.pac.chksum += it;
    // }
    fdata.push_back(fpac);
  } while (!f.eof());
  idx = -1;
  header.push_back(FILE_VERSION);
  header.push_back(ftype);
  header.push_back(fileName.size());
  header.insert(header.end(), fileName.begin(), fileName.end());
  header.push_back(fdata.size());
  // uint8_t sum = 0;
  // for (auto&& it : header) sum += it;
  // header.push_back(sum);
  return true;
}
vector<uint8_t> ASAFileEnc::get(bool next) {
  vector<uint8_t> buf;
  if (next) idx++;
  if (idx < 0)
    buf = header;
  else if (idx == fdata.size())
    buf = vector<uint8_t>();
  else
    buf = vector(fdata[idx].raw.begin(), fdata[idx].raw.end());
  return buf;
}
void ASAFileEnc::clear() {
  ftype = FILETYPE::BIN;
  idx = 0;
  header.clear();
  fdata.clear();
  isHeader = true;
}

// ASA File Decode
ASAFileDec::ASAFileDec() {}

ASAFileDec::~ASAFileDec() {}

bool ASAFileDec::put(uint8_t buff) {
  static uint16_t dlen = 0;
  // if (skip) return false;
  // if (isProcessing) {
  switch (decodeState) {
    case FSTATE::f_ver:
      if (buff != 1) return false;  // version
      decodeState++;
      break;
    case FSTATE::f_type:
      filetype = buff;
      decodeState++;
      break;
    case FSTATE::f_name_len:
      dlen = buff;
      fileName.resize(dlen);
      decodeState++;
      break;
    case FSTATE::f_name:
      fileName[count++] = buff;
      if (count == dlen) {
        count = 0;
        decodeState++;
      }
      break;
    case FSTATE::sector_num:
      sectorNum = buff;
      decodeState++;
      break;
    case FSTATE::sector_idx:
      sectorIdx = buff;
      decodeState++;
      break;
    case FSTATE::sector_dat:
      buffer[count++] = buff;
      if (count == 256) {
        if (sectorNum - 1 == sectorIdx) {
          decodeState = FSTATE::f_type;
          fend = true;
        } else
          decodeState++;
      }
      break;
  }
  // }
  return true;
}

bool ASAFileDec::save() {
  static bool first = true;
  if (first) {
    first = false;
    return false;
  }
  std::ios_base::openmode mode = ios::app | ios::in;
  if (filetype == BIN) mode |= ios::binary;
  fstream f(fileName, mode);
  if (filetype == BIN)
    f.write(reinterpret_cast<char *>(buffer.data()), buffer.size());
  else {
    string str(buffer.begin(), buffer.end());
    str.erase(str.find_last_not_of((char)0) + 1);
    f.write(str.data(), str.size());
  }
  f.close();
  if (fend) {
    clear();
    first = true;
    return true;
  }
  return false;
}

void ASAFileDec::clear() {
  decodeState = FSTATE::f_ver;
  buffer.fill(0);
  count = sectorIdx = 0;
  fend = false;
  fileName.clear();
}
}  // namespace ASAEncoder