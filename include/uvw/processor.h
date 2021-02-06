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

    public:
    
    Processor();
    virtual ~Processor();

    virtual bool initialize() {return true;}
    virtual bool preprocess() {return true;}
    virtual bool process() {return true;}

    template<typename T>
    bool reg_var(const std::string& label, Var<T>& var);
    template<typename T>
    T& ref(const std::string& label);
    Variable* get(const std::string& label);
  };

};

// implementation

template<typename T>
bool uvw::Processor::reg_var(const std::string& label, Var<T>& v)
{
  Duo key(this, label);
  v.key_ = key;
  v.data_ptr_ = &v.value_;

  if (key.is_null())
  {
    std::cout << "Failure: invalid empty label." << std::endl;
    return false;
  }

  if (uvw::ws::has(key))
  {
    std::cout << "Warning: var " << label << " exists." << std::endl;
    return false;
  }

  if (!uvw::ws::add<T>(key, v))
  {
    std::cout << "Failure: Unable to add var " << label << std::endl;
    return false;
  }

  var_keys_.insert(key);

  return true;
}

template<typename T> T& uvw::Processor::ref(const std::string& label)
{
  return uvw::ws::ref<T>(uvw::Duo(this, label));
}

#endif