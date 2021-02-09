#ifndef UVW_VARIABLE_H
#define UVW_VARIABLE_H

#include "duohash.h"

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

    Duohash key_;
    Duohash src_;
    size_t type_; // std::typeinfo::hash_code()
    void* data_ptr_;
    void* data_src_;

    public:

    enum Dimension
    {
        DYN = -1,   // Dynamic size
        NAA = 0,    // Not an array
        SCALAR = 1, // 1-D vectors
        VECTOR = 3, // 3-D vectors
        MATRIX = 16 // Array of matrices
    };

    enum Parameter
    {
      NONPARAM = 0,
      OUTPUT,
      INPUT
    };

    Dimension dimension;
    Parameter parameter;

    protected:

    void init()
    {
      data_ptr_ = data_src_ = nullptr;
      parameter = NONPARAM;
      dimension = NAA;
    }

    void copy(const Variable& v)
    {
      data_ptr_ = v.data_ptr_;
      data_src_ = v.data_src_;
      parameter = v.parameter;
      dimension = v.dimension;
      key_ = v.key_;
      src_ = v.src_;
    }

    Variable(
      const Duohash& key,
      size_t type_code = 0
    ): key_(key), type_(type_code) {init();}

    public:
    Variable(): type_(0) {init();}
    virtual ~Variable() {init();}
    Variable(const Variable& v) {*this = v;}
    Variable& operator=(const Variable& v) {copy(v); return *this;}

    const Duohash& key() {return key_;}
    const std::string label() {return key_.var_str;}
    Processor* proc();

    template<typename T> bool is_of_type();
    template<typename T> static bool is_null(const T& obj);

    void unlink() {src_ = Duohash(nullptr,""); data_src_ = nullptr;}
    bool link(Variable* src);
    const Duohash src() {return src_;}

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

    Var(const Duohash& key):
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

  using var = Variable;
};

#endif
