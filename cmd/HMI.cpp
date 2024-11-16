#include "HMI.h"

#include <algorithm>
#include <format>

#include "asaEncoder.h"
using namespace ASAEncoder;
int Object::col;
std::vector<Object *> Object::objs;

Object::Object(TYPE type, std::string str) : type(type) {
  _name = ASABasic::pacTypeStr(type) + std::to_string(objs.size());
  int pos = str.find('\n');
  this->format = str.substr(0, pos - 1);
  this->detail = str.substr(pos + 1);
  objs.push_back(new Object(*this));
}

Object::Object(std::string name, std::string detail) {
  _name = name;
  if (name.find("MT") >= 0)
    type = TYPE::MATRIX;
  else if (name.find("ST") >= 0)
    type = TYPE::STRUCT;
  ASAEncode enc;
  if (!enc.put(detail)) return;
  int pos = detail.find('\n');
  this->format = detail.substr(0, pos - 1);
  this->detail = detail.substr(pos + 1);
  // format = enc.getFormat();
  // detail = enc.detailStr();
  objs.push_back(new Object(*this));
}

Object::~Object() {
  auto find = std::find(objs.begin(), objs.end(), this);
  if (find != objs.end()) {
    delete (*find);
    objs.erase(find);
  }
}
std::string Object::getAllVisible() {
  std::string str;
  for (auto obj : objs) {
    // if (obj->type == TYPE::FILE) continue;
    str += obj->getVisible();
  }
  return str;
}

std::string Object::getStr() {
  std::string str;
  str = getName() + "\n" + getFormat() + "\n" + getDetail();
  return str;
}

Object *Object::getObj(int index) {
  if (index < objs.size())
    return objs[index];
  else
    return nullptr;
}

Object *Object::getObj(string name) {
  auto f = [&](const auto &o) { return name == o->getName(); };
  auto o = std::find_if(objs.begin(), objs.end(), f);
  // vector<Object *>::iterator o = objs.begin();
  if (o != objs.end())
    return *o;
  else
    return nullptr;
}

void Object::setCol(int col) { Object::col = col; }

std::string Object::getVisible() {
  std::string str =
      std::format("{0:^5}|{1:^{3}}|{2:^{3}}\n", ASADecode::pacTypeStr(type),
                  _name, format, col / 2 - 5);
  return str;
}
std::string Object::getName() const { return _name; }
std::string Object::getFormat() const { return format; }
std::string Object::getDetail() const { return detail; }

bool Object::change(std::string str) {  // TODO: text check
  // auto things = ASAEncode::split(str);
  // if (things.empty()) return false;
  // int pos = things[0].str.find(':');
  // int pos = things[0].str.find(':');
  // std::string newName = str.substr(0, str.find('\n') - 1);
  // std::string newFormat = things[0].str.substr(0, pos - 1);
  // std::string newDetail = things[0].str.substr(pos + 1);
  ASAEncode enc;

  int pos = str.find('\n');
  std::string newName = str.substr(0, pos - 1);
  std::string others = str.substr(pos);
  if (!enc.put(others)) return false;
  _name = newName;
  format = enc.getFormat();
  detail = enc.detailStr();
  return true;
}

void Object::renName(std::string newName) { _name = newName; }
void Object::renDetail(std::string newDetail) { detail = newDetail; }