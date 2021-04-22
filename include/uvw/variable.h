#ifndef UVW_VARIABLE_H
#define UVW_VARIABLE_H

#include "duohash.h"

#include <unordered_set>
#include <unordered_map>
#include <typeindex>

#include <json.hpp>
using json = nlohmann::json;


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
    void* data_ptr_;
    void* data_src_;

    public:

    bool enabled;

    static bool data_pull;

    enum Dimension
    {
      DIM_DYN = -1,   // Dynamic size
      DIM_NAA = 0,    // Not an array
      DIM_SCALAR = 1, // 1-D vectors
      DIM_VECTOR = 3, // 3-D vectors
      DIM_MATRIX = 16 // Array of 4x4 matrices
    };

    enum Permission
    {
      PER_HIDDEN = 0,
      PER_READABLE,
      PER_WRITABLE
    };

    enum Parameter
    {
      PAR_NONPARAM = 0,
      PAR_OUTPUT,
      PAR_INPUT
    };

    std::unordered_map<std::string, int> properties;

    protected:

    void init()
    {
      incoming_.clear();
      enabled = true;
      data_ptr_ = data_src_ = nullptr;
      properties = {
        {"parameter", PAR_NONPARAM},
        {"dimension", DIM_NAA},
        {"permission", PER_WRITABLE}
      };
    }

    void copy(const Variable& v)
    {
      incoming_ = v.incoming_;
      enabled = v.enabled;
      data_ptr_ = v.data_ptr_;
      data_src_ = v.data_src_;
      key_ = v.key_;
      src_ = v.src_;
      properties = v.properties;
    }

    Variable(const Duohash& key): key_(key) {init();}

    public:
    Variable() {init();}
    virtual ~Variable() {init();}
    Variable(const Variable& v) {*this = v;}
    Variable& operator=(const Variable& v) {copy(v); return *this;}

    const Duohash& key() {return key_;}
    const std::string label() {return key_.var_str;}
    Processor* proc();
    void* raw_data() {return data_ptr_;}

    template<typename T> bool is_of_type();
    template<typename T> static bool is_null(const T& obj);

    bool unlink();
    bool link(Variable* src);
    const Duohash src() {return src_;}

    virtual void pull() = 0;
    virtual const std::type_index type_index() = 0;

    virtual json to_json();
    virtual bool from_json(const json& data);

    const std::string type_str();
    static std::map<std::type_index, std::string> type_strs;

    protected:
    void propagate(void* data_src);
    std::unordered_set<Duohash> incoming_;
    template<class T> static T null_;
  };

  template<class T>
  class Var : public Variable
  {
    friend class Workspace;
    friend class Processor;

    protected:

    T value_;

    Var(const Duohash& key):
      Variable(key) {}

    public:

    Var(): Variable() {}

    const std::type_index type_index() override
    {
      return std::type_index(typeid(T));
    }

    void pull() override
    {
      if (data_src_)
      {
        value_ = *((T*)data_src_);
      }
    }

    // json values
    std::unordered_map<std::string, T> values;

    json to_json() override
    {
      json data = Variable::to_json();
      for (auto& itr : values)
      {
        data["values"][itr.first] = itr.second;
      }
      data["value"] = get();
      return data;
    }

    bool from_json(const json& data) override
    {
      if (data.find("values") != data.end())
      {
        for (auto& itr : data["values"].items())
        {
          values[itr.key()] = itr.value().get<T>();
        }
      }

      if (data.find("value") != data.end())
      {
        value_ = data["value"].get<T>();
      }
      return Variable::from_json(data);
    }

    // type-specific members
    T& ref()
    {
      return (data_pull || data_src_ == nullptr)?
        value_ : *((T*)data_src_);
    }
    T& operator()() {return ref();}
    T get() {return ref();}
    void set(const T& val) {value_ = val;}
  };

  // impl.

  template<class T> T Variable::null_;
  template<typename T> bool Variable::is_null(const T& obj)
  {
    return &obj == &Variable::null_<T>;
  }
  template<typename T> bool Variable::is_of_type()
  {
    return (std::type_index(typeid(T)) == type_index());
  }

  using var = Variable;
};

#endif
