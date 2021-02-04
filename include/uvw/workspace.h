#ifndef UVW_WORKSPACE_H
#define UVW_WORKSPACE_H

#include <unordered_map>
#include <set>
#include <iostream>

#include "duohash.h"
#include "variable.h"

namespace uvw
{
  class Processor;

  class Workspace
  {
    friend class Variable;
    friend class Processor;

    protected:

    static std::unordered_map<Duo, Variable*> vars_;

    public:

    static bool has(const Duo& key)
    {
      return (vars_.find(key) != vars_.end());
    }

    static bool del(const Duo& key)
    {
      if (has(key))
      {
        vars_.erase(key);
        return true;
      }
      return false;
    }

    static Variable* get(const Duo& key)
    {
      return has(key)? vars_[key]:nullptr;
    }

    template<typename T>
    static bool add(const Duo& key, Var<T>& v)
    {
      if (has(key))
      {
        std::cout << "Warning: var " << key << " exists!" << std::endl;
        return false;
      }
      vars_[key] = &v;
      return true;
    }

    template<typename T>
    static T& ref(const Duo& key)
    {
      if (has(key) && key.raw_ptr)
      {
        void* data_ptr = vars_[key]->data_ptr_;
        if (data_ptr)
        {
          return *((T*)data_ptr);
        }
      }
      return Variable::null_<T>;
    }

    static bool link(const Duo& src, const Duo& dst)
    {
      if (!has(src) || !has(dst))
      {
        return false;
      }
      return vars_[dst]->link(vars_[src]);
    }

    static std::vector<Duo> schedule(const Duo& key);
    static bool execute(const std::vector<Duo>& seq, bool preprocess = false);

    protected:

    static std::set<Processor*> procs_;

    static bool exists_(Processor* proc_ptr)
    {
      return (procs_.find(proc_ptr) != procs_.end());
    }

    static bool track_(Processor* proc_ptr)
    {
      if (!exists_(proc_ptr))
      {
        procs_.insert(proc_ptr);
        return true;
      }
      return false;
    }

    static bool untrack_(Processor* proc_ptr)
    {
      if (exists_(proc_ptr))
      {
        return procs_.erase(proc_ptr);
      }
      return false;
    }

    public:

    static std::unordered_map<Duo, Variable*>& vars() {return vars_;}
    static std::set<Processor*>& procs() {return Workspace::procs_;}

  };

  std::unordered_map<Duo, Variable*> Workspace::vars_;
  std::set<Processor*> Workspace::procs_;
  using ws = Workspace;
};

#endif