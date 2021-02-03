#ifndef UVW_PROCESSOR_H
#define UVW_PROCESSOR_H

#include <unordered_set>
#include <iostream>


namespace uvw
{
  class Workspace;

  class Processor
  {
    friend class Workspace;

    protected:

    std::unordered_set<Duo> var_keys_;

    template<typename T>
    bool del_var(const std::string& label)
    {
      return uvw::ws::del<T>(Duo(this, label));
    }

    public:
    
    Processor();
    virtual ~Processor();

    virtual bool initialize() {return false;}
    virtual bool preprocess() {return false;}
    virtual bool process() {return false;}
    virtual void cleanup() {} // where del_var takes place...

    template<typename T>
    bool new_var(const std::string& label, T& var);
    template<typename T>
    T& var(const std::string& label);
  };
};

// implementation

#include "duohash.h"
#include "variable.h"

uvw::Processor::Processor()
{
  uvw::ws::track_(this);
}

uvw::Processor::~Processor()
{
  uvw::ws::untrack_(this);
  cleanup();
}

template<typename T>
T& uvw::Processor::var(const std::string& label)
{
  return uvw::ws::get<T>(Duo(this, label));
}

template<typename T>
bool uvw::Processor::new_var(const std::string& label, T& var)
{
  Duo key(this, label);
  if (key.is_null())
  {
    std::cout << "Failure: invalid empty label." << std::endl;
    return false;
  }

  if (uvw::ws::has_var(key))
  {
    std::cout << "Warning: var " << label << " exists." << std::endl;
    return false;
  }

  if (!uvw::ws::add<T>(key, &var))
  {
    std::cout << "Failure: Unable to add var " << label << std::endl;
    return false;
  }

  var_keys_.insert(key);

  return true;
}

uvw::Processor* uvw::Variable::proc()
{
  if (key_.ptr)
  {
    return static_cast<Processor*>(key_.ptr);
  }
  return nullptr;
}

#endif