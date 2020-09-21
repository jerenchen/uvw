#ifndef UVW_VARIABLE_H
#define UVW_VARIABLE_H

#include <unordered_map>
#include <iostream>
#include <typeinfo>

namespace uvw
{
  class Workspace;

  class Variable
  {
    friend class Workspace;

    std::string label_;
    size_t type_; // std::typeinfo::hash_code()

    public:

    Variable(
      const std::string& label = "",
      size_t type_code = 0
    ): label_(label), type_(type_code) {}

    template<typename T> bool is_of_type();
    template<typename T> static bool is_null(const T& obj);

    protected:
    template<class T> static T null_;
    template<class T> static size_t var_type;
  };

  template<class T> T Variable::null_;
  template<class T> size_t Variable::var_type = typeid(Variable::null_<T>).hash_code();
  template<typename T> bool Variable::is_null(const T& obj)
  {
    return &obj == &Variable::null_<T>;
  }
  template<typename T> bool Variable::is_of_type()
  {
    return (var_type<T> == type_);
  }

  using var = Variable;
};

#endif