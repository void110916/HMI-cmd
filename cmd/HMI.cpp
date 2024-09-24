#include "HMI.h"

#include <format>
#include <algorithm>
#include "asaEncoder.h"
using namespace ASAEncoder;
int Object::col;
std::vector<Object *> Object::objs;

Object::Object(TYPE type, std::string str) : type(type) {
  name = ASADecode::getTypeStr(type) + std::to_string(objs.size());
  int pos = str.find('\n');
  format = str.substr(0, pos - 1);
  detail = str.substr(pos + 1);
  objs.push_back(new Object(*this));
}

Object::~Object() {
  auto find = std::find(objs.begin(), objs.end(), this);
  delete(*find);
  if (find != objs.end()) objs.erase(find);
}
std::string Object::getAllVisible() {
  std::string str;
  for (auto obj : objs) {
    // if (obj->type == TYPE::FILE) continue;
    str += obj->getVisible() + "\n";
  }
  return str;
}

Object *Object::getObj(int index) {
  if (index < objs.size())
    return objs[index];
  else
    return nullptr;
}

Object *Object::getObj(string std) {
  // auto f=[&](const auto& o){return std==o->getName();};
  // auto o= std::find_if(objs.begin(),objs.end(),f);
  vector<Object *>::iterator o=objs.begin();
  if (o!=objs.end())
    return *o;
  else
    return nullptr;
}

void Object::setCol(int col) { Object::col = col; }

std::string Object::getVisible() {
  std::string str =
      std::format("{0:^4}|{1:^{3}}|{2:^{3}}\n", ASADecode::getTypeStr(type),
                  name, format, col / 2 - 4);
  return str;
}
std::string Object::getName() const { return name; }
std::string Object::getFormat() const { return format; }
std::string Object::getDetail() const { return detail; }

bool Object::change(std::string str) {
  // auto things = ASAEncode::split(str);
  // if (things.empty()) return false;
  // int pos = things[0].str.find(':');
  // int pos = things[0].str.find(':');
  // std::string newName = str.substr(0, str.find('\n') - 1);
  // std::string newFormat = things[0].str.substr(0, pos - 1);
  // std::string newDetail = things[0].str.substr(pos + 1);
  ASAEncode enc;
  if(!enc.put(str)) return false;
  int pos = str.find(':');
  std::string newName = str.substr(0, str.find('\n') - 1);
  std::string newFormat = str.substr(0, pos - 1);
  std::string newDetail = str.substr(pos + 1);
  return true;
}

void Object::renName(std::string newName) { name = newName; }
void Object::renDetail(std::string newDetail) { detail = newDetail; }