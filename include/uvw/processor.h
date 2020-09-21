#ifndef UVW_PROCESSOR_H
#define UVW_PROCESSOR_H

#include <iostream>

namespace uvw
{
  class Workspace;

  class Processor
  {
    friend class Workspace;

    std::string label_; 

    public:
    
    Processor(const std::string& label = "");
    virtual ~Processor();

    virtual bool initialize() {return false;}
    virtual bool preprocess() {return false;}
    virtual bool process() {return false;}

    template<typename T>
    bool new_var(const std::string& label);
    template<typename T>
    T& var(const std::string& label);
  };
};

// implementation

#include "variable.h"

uvw::Processor::Processor(const std::string& label): 
  label_(label)
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
  return uvw::ws::get<T>(label);
}

template<typename T>
bool uvw::Processor::new_var(const std::string& label)
{
  if (uvw::ws::has_var(label))
  {
    std::cout << "Warning: var " << label << " exists." << std::endl;
    return false;
  }

  if (!uvw::ws::add<T>(label))
  {
    std::cout << "Failure: Unable to add var " << label << std::endl;
    return false;
  }

  return true;
}

#endif