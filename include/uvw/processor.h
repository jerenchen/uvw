#ifndef UVW_PROCESSOR_H
#define UVW_PROCESSOR_H

#include <iostream>

namespace uvw
{
  class Workspace;

  class Processor
  {
    friend class Workspace;

    public:
    
    Processor();
    virtual ~Processor();

    virtual bool initialize() {return false;}
    virtual bool preprocess() {return false;}
    virtual bool process() {return false;}

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
}

template<typename T>
T& uvw::Processor::var(const std::string& label)
{
  return uvw::ws::get<T>(Duo(this, label));
}

template<typename T>
bool uvw::Processor::new_var(const std::string& label, T& var)
{
  if (uvw::ws::has_var(Duo(this, label)))
  {
    std::cout << "Warning: var " << label << " exists." << std::endl;
    return false;
  }

  if (!uvw::ws::add<T>(Duo(this, label), &var))
  {
    std::cout << "Failure: Unable to add var " << label << std::endl;
    return false;
  }

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