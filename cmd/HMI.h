#include <string>
#include <vector>
class Object {
 public:
  enum TYPE:int { MATRIX = 2, STRUCT = 3 };  // , FILE = 4

 private:
  static std::vector<Object *> objs;
  static int col;
  TYPE type;
  std::string name;
  std::string format;
  std::string detail;

 public:
  Object(TYPE type, std::string str);
  ~Object();
  static std::string getAllVisible();
  static Object *getObj(int index);
  static void setCol(int col);
  std::string getName() const;
  std::string getFormat() const;
  std::string getDetail() const;
  std::string getVisible();
  void renName(std::string newTitle);
  void renDetail(std::string newDetail);
};
