#include "HMI.h"

#include <format>

#include "asaEncoder.h"
using namespace ASAEncoder;
int Object::col;
std::vector<Object *> Object::objs;

Object::Object(TYPE type, std::string str) : type(type) {
  name = ASADecode::getPacTypeStr(type) + std::to_string(objs.size());
  int pos = str.find('\n');
  format = str.substr(0, pos - 1);
  detail = str.substr(pos + 1);
  objs.push_back(new Object(*this));
}

Object::~Object() {
  auto find = std::find(objs.begin(), objs.end(), this);
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

void Object::setCol(int col) { Object::col = col; }

std::string Object::getVisible() {
  std::string str =
      std::format("{0:^4}|{1:^{3}}|{2:^{3}}\n", ASADecode::getPacTypeStr(type),
                  name, format, col / 2 - 4);
  return str;
}
std::string Object::getName() const { return name; }
std::string Object::getFormat() const { return format; }
std::string Object::getDetail() const { return detail; }
void Object::renName(std::string newName) { name = newName; }
void Object::renDetail(std::string newDetail) { detail = newDetail; }