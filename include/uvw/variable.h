#ifndef UVW_VARIABLE_H
#define UVW_VARIABLE_H

#include "duohash.h"

#include <unordered_set>
#include <unordered_map>
#include <typeindex>

#include <picojson.h>
using json = picojson::value;


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

    static bool data_pull;

    bool enabled;
    std::unordered_map<std::string, int> properties;

    protected:

    void init()
    {
      incoming_.clear();
      enabled = true;
      data_ptr_ = data_src_ = nullptr;
      properties.clear();
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
    virtual bool from_json(json& data);

    const std::string type_str();
    static std::map<std::type_index, std::string> type_strs;

    virtual bool set_enum(const std::string& key) = 0;
    virtual const std::string default_enum() {return "";}
    virtual const std::vector<std::string> enum_keys() {return {};}

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

    // type-specific members
    std::unordered_map<std::string, T> values;

    T& ref()
    {
      return (data_pull || data_src_ == nullptr)?
        value_ : *((T*)data_src_);
    }
    T& operator()() {return ref();}
    T get() {return ref();}
    void set(const T& val) {value_ = val;}
    const T& default_value()
    {
      return (values.find("default") != values.end()?
                values["default"] : Variable::null_<T>);
    }

    // enums
    std::vector<std::pair<std::string, T> > enums;

    const T& operator[](const std::string& key)
    {
      for (auto& itr : enums)
      {
        if (itr.first == key)
        {
          return itr.second;
        }
      }
      return default_value();
    }

    bool set_enum(const std::string& key) override
    {
      for (auto& itr : enums)
      {
        if (itr.first == key)
        {
          value_ = itr.second;
          return true;
        }
      }
      return false;
    }

    const std::string default_enum() override
    {
      auto& dv = default_value();
      if (!is_null(dv))
      {
        for (auto& itr : enums)
        {
          if (itr.second == dv)
          {
            return itr.first;
          }
        }
      }
      return enums.size()? enums[0].first : std::string();
    }

    const std::vector<std::string> enum_keys() override
    {
      std::vector<std::string> keys;
      for (auto& itr : enums)
      {
        keys.push_back(itr.first);
      }
      return keys;
    }

    // json serialize

    json to_json() override
    {
      json data = Variable::to_json();
      auto& data_obj = data.get<json::object>();

      if (enums.size())
      {
        json::array enum_array;
        for (auto& itr : enums)
        {
          json::object elem;
          elem["key"] = json(itr.first);
          elem["value"] = json(itr.second);
          enum_array.push_back(json(elem));
        }
        data_obj["enums"] = json(enum_array);
      }

      if (values.size())
      {
        json::object value_obj;
        for (auto& itr : values)
        {
          value_obj[itr.first] = json(itr.second);
        }
        data_obj["values"] = json(value_obj);
      }

      data_obj["value"] = json(get());

      return data;
    }

    bool from_json(json& data) override
    {
      auto& data_obj = data.get<json::object>();

      enums.clear();
      if (data_obj.find("enums") != data_obj.end() &&
            data_obj["enums"].is<json::array>())
      {
        for (auto& itr : data_obj["enums"].get<json::array>())
        {
          auto enum_obj = itr.get<json::object>();
          enums.push_back(
            {
              enum_obj["key"].get<std::string>(),
              enum_obj["value"].get<T>()
            }
          );
        }
      }

      values.clear();
      if (data_obj.find("values") != data_obj.end())
      {
        for (auto& itr : data_obj["values"].get<json::object>())
        {
          values[itr.first] = itr.second.get<T>();
        }
      }

      if (data_obj.find("value") != data_obj.end())
      {
        value_ = data_obj["value"].get<T>();
      }

      return Variable::from_json(data);
    }
  };

  // macro to define specialized overrides without values/enums
  #define UVW_VAR_SPECIALIZE_DEFAULT(x) \
    template<> const std::string uvw::Var<x>::default_enum()\
        {return Variable::default_enum();}\
    template<> bool uvw::Var<x>::from_json(json& data)\
        {return Variable::from_json(data);}\
    template<> json uvw::Var<x>::to_json()\
        {return Variable::to_json();}

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
