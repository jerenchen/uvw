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

    protected:

    Duo key_;
    Duo src_;
    size_t type_; // std::typeinfo::hash_code()
    void* data_ptr_;
    void* data_src_;

    Variable(
      const Duo& key,
      size_t type_code = 0
    ): key_(key), type_(type_code)
    {
      data_ptr_ = data_src_ = nullptr;
    }

    public:
    Variable(): type_(0)
    {
      data_ptr_ = data_src_ = nullptr;
    }

    const Duo& key() {return key_;}
    const std::string label() {return key_.var_str;}
    Processor* proc();

    template<typename T> bool is_of_type();
    template<typename T> static bool is_null(const T& obj);

    void unlink() {src_ = Duo(nullptr,""); data_src_ = nullptr;}
    bool link(Variable* src);
    const Duo src() {return src_;}

    virtual void pull() {}

    protected:
    template<class T> static T null_;
    template<class T> static size_t var_type;
  };

  template<class T>
  class Var : public Variable
  {
    friend class Workspace;
    friend class Processor;

    protected:

    T value_;

    Var(const Duo& key):
      Variable(key, typeid(T).hash_code()) {}

    public:

    Var(): Variable() {}

    void pull() override
    {
      if (data_src_)
      {
        value_ = *((T*)data_src_);
      }
    }

    // type-specific members
    T& ref() {return value_;}
    T& operator()() {return value_;}
    void set(const T& val) {value_ = val;}
    T get() {return value_;}
  };

  // impl.

  template<class T> T Variable::null_;
  template<class T> size_t Variable::var_type = typeid(T).hash_code();
  template<typename T> bool Variable::is_null(const T& obj)
  {
    return &obj == &Variable::null_<T>;
  }
  template<typename T> bool Variable::is_of_type()
  {
    return (var_type<T> == type_);
  }

  bool Variable::link(Variable* src)
  {
    if (type_ !=0 && type_ != src->type_)
    {
      unlink();
      return false;
    }
    // hook up key & src data ptr
    src_ = Duo(src->key());
    data_src_ = src->data_ptr_;
    return true;
  }

  using var = Variable;
};

#endif