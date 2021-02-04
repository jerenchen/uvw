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

#include <queue>
#include <stack>

std::vector<uvw::Duo> uvw::Workspace::schedule(const uvw::Duo& key)
{
  std::vector<uvw::Duo> res;
  if (!uvw::Workspace::has_var(key))
  {
    return res;
  }

  std::queue<uvw::Processor*> queue_proc;
  queue_proc.push(uvw::Workspace::all_vars_[key].proc());

  std::stack<uvw::Duo> stack_vkey;
  while (queue_proc.size())
  {
    uvw::Processor* p_ = queue_proc.front();
    queue_proc.pop();

    for (auto& k_ : p_->var_keys_)
    {
      if (!uvw::Workspace::has_var(k_))
      {
        continue;
      }

      auto& var = uvw::Workspace::all_vars_[k_];

      if (uvw::Workspace::has_var(var.source()))
      {
        auto& src = uvw::Workspace::all_vars_[var.source()];
        uvw::Processor* dep_proc = src.proc();
        if (dep_proc)
        {
          queue_proc.push(dep_proc);
          stack_vkey.push(k_);
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
  std::unordered_set<void*> visited_procs;

  uvw::Processor* curr = nullptr;
  for (const auto& key : seq)
  {
    // Assuming var & its source are validated by schedule...
    auto& var = uvw::Workspace::all_vars_[key];
    auto* proc = uvw::Workspace::all_vars_[var.source()].proc();
    if (visited_procs.find(proc) == visited_procs.end())
    {
      // TODO: option to terminate if process failed
      proc->process();
      visited_procs.insert(proc);
    }
    // var is ready to pull
    std::cout << "Pulling var " << var.label() << std::endl;
    // ...
    curr = var.proc();
  }

  if (uvw::Workspace::exists_(curr) &&
    visited_procs.find(curr) == visited_procs.end())
  {
    curr->process();
  }

  return true;
}

#endif