#ifndef UVW_VARIABLE_H
#define UVW_VARIABLE_H

#include "duohash.h"

#include <unordered_map>
#include <iostream>
#include <typeinfo>


namespace uvw
{
  class Workspace;
  class Processor;

  class Variable
  {
    friend class Workspace;
    friend class Processor;

    Duo key_;
    size_t type_; // std::typeinfo::hash_code()

    protected:

    Variable(
      const Duo& key,
      size_t type_code = 0
    ): key_(key), type_(type_code) {}

    public:
    Variable(): type_(0) {}

    const Duo& key() {return key_;}
    const std::string label() const {return key_.var;}
    Processor* proc();

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