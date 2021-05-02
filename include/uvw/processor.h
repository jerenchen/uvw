#ifndef UVW_PROCESSOR_H
#define UVW_PROCESSOR_H

#include "duohash.h"
#include "variable.h"

#include <unordered_set>


namespace uvw
{
  class Workspace;

  class Processor
  {
    friend class Workspace;

    protected:

    std::vector<Duohash> var_keys_;
    std::string type_;

    public:
    
    Processor();
    Processor(const Processor& p);
    virtual ~Processor();

    Processor& operator=(const Processor& p);

    virtual bool initialize() {return true;}
    virtual bool preprocess() {return true;}
    virtual bool process(bool preprocess=false) {return true;}

    template<typename T>
    bool reg_var(const std::string& label, Var<T>& var);
    template<typename T>
    T& ref(const std::string& label);
    Variable* get(const std::string& label);

    json to_json();
    bool from_json(json& data);

    const std::string& type_str() const {return type_;}
    const std::vector<Duohash>& var_keys() const {return var_keys_;}
  };

};

// implementation
#include <iostream>

template<typename T>
bool uvw::Processor::reg_var(const std::string& label, Var<T>& v)
{
  Duohash key(this, label);
  v.key_ = key;
  v.data_ptr_ = &v.value_;

  if (key.is_null())
  {
    std::cout << "Failure: invalid empty label." << std::endl;
    return false;
  }

  if (uvw::ws::has(key))
  {
    std::cout << "Warning: var '" << label << "' exists." << std::endl;
    return false;
  }

  if (!uvw::ws::add_<T>(v))
  {
    std::cout << "Failure: Unable to add var '" << label << "'" << std::endl;
    return false;
  }

  var_keys_.push_back(key);

  return true;
}

template<typename T> T& uvw::Processor::ref(const std::string& label)
{
  return uvw::ws::ref<T>(uvw::Duohash(this, label));
}

#endif