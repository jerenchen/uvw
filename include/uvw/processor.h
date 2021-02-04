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

    virtual bool initialize() {return false;}
    virtual bool preprocess() {return false;}
    virtual bool process() {return false;}

    template<typename T>
    bool reg_var(const std::string& label, Var<T>& var);
    template<typename T>
    T& ref(const std::string& label);
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
  // de-register proc & vars
  uvw::ws::untrack_(this);
  for (auto& key : var_keys_)
  {
    uvw::ws::del(key);
  };
}

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

uvw::Processor* uvw::Variable::proc()
{
  if (key_.raw_ptr)
  {
    return static_cast<Processor*>(key_.raw_ptr);
  }
  return nullptr;
}

#include <queue>
#include <stack>

std::vector<uvw::Duo> uvw::Workspace::schedule(const uvw::Duo& key)
{
  std::vector<uvw::Duo> res;
  if (!uvw::Workspace::has(key))
  {
    return res;
  }

  std::queue<uvw::Processor*> queue_proc;
  uvw::Processor* proc = uvw::Workspace::vars_[key]->proc();
  if (!proc)
  {
    std::cout << "Warning: null proc found for " << key << std::endl;
    return res;
  }
  queue_proc.push(proc);

  std::stack<uvw::Duo> stack_vkey;
  while (queue_proc.size())
  {
    proc = queue_proc.front();
    queue_proc.pop();

    for (auto& vk : proc->var_keys_)
    {
      if (!uvw::Workspace::has(vk))
      {
        continue;
      }
      Variable* var = uvw::Workspace::vars_[vk];

      if (uvw::Workspace::has(var->src()))
      {
        Variable* src = uvw::Workspace::vars_[var->src()];
        uvw::Processor* src_proc = src->proc();
        if (src_proc)
        {
          queue_proc.push(src_proc);
          stack_vkey.push(vk);
        }
      }
    }
  }

  while (stack_vkey.size())
  {
    res.push_back(stack_vkey.top());
    stack_vkey.pop();
  }
  return res;
}

bool uvw::Workspace::execute(const std::vector<uvw::Duo>& seq)
{
  std::unordered_set<void*> marked_processed;

  uvw::Variable* var = nullptr;
  for (const auto& key : seq)
  {
    // Assuming var & its src are validated by schedule...
    var = uvw::Workspace::vars_[key];
    auto* proc = uvw::Workspace::vars_[var->src()]->proc();
    if (marked_processed.find(proc) == marked_processed.end())
    {
      if (!proc->process())
      {
        return false;
      }
      marked_processed.insert(proc);
    }
    // var is ready to pull
    var->pull();
  }

  if (var && uvw::Workspace::exists_(var->proc()))
  {
    auto* proc = var->proc();
    if (marked_processed.find(proc) == marked_processed.end())
    {
      proc->process();
    }
  }

  return true;
}

#endif