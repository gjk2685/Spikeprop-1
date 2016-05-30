// File: convert.h
// From Marshall Cline's C++ FAQ Lite document, www.parashift.com/c++-faq-lite/".
// ([38.3] Can I templatize the above functions so they work with other types?)

#include <iostream>
#include <sstream>
#include <string>
#include <typeinfo>
#include <stdexcept>

class BadConversion : public std::runtime_error {
public:
  BadConversion(const std::string& s)
  : std::runtime_error(s)
  { }
};

template<typename T>
inline std::string stringify(const T& x)
{
  std::ostringstream o;
  if (!(o << x))
    throw BadConversion(std::string("stringify(")
      + typeid(x).name() + ")");
  return o.str();
} 
/*
int main(void)
{
  double x = 229;
  std::string s = "the value is " + stringify(x);
  cout << s << endl;
}
*/
