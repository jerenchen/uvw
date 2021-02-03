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
    friend class Processor;

    protected:

    static std::unordered_map<Duo, Variable> all_vars_;
    template<class T> static std::unordered_map<Duo, T*> vars_;

    public:

    static bool has_var(const Duo& key)
    {
      return (all_vars_.find(key) != all_vars_.end());
    }

    template<typename T>
    static bool has(const Duo& key)
    {
      return (vars_<T>.find(key) != vars_<T>.end());
    }

    template<typename T>
    static bool del(const Duo& key)
    {
      if (vars_<T>.find(key) == vars_<T>.end())
      {
        return false;
      }

      vars_<T>.erase(key);

      if (has_var(key))
      {
        all_vars_.erase(key);
      }

      return true;
    }

    template<typename T>
    static T& get(const Duo& key)
    {
      return (has<T>(key) && vars_<T>[key])? 
        *(vars_<T>[key]) : Variable::null_<T>;
    }

    template<typename T>
    static bool add(const Duo& key, T* var)
    {
      if (has_var(key))
      {
        return false;
      }

      vars_<T>[key] = var;
      all_vars_[key] = Variable(key, Variable::var_type<T>);

      return true;
    }

    protected:

    static std::set<Processor*> procs_;

    static bool exists_(Processor* ptr)
    {
      return (procs_.find(ptr) != procs_.end());
    }

    static bool track_(Processor* ptr)
    {
      if (!exists_(ptr))
      {
        procs_.insert(ptr);
        return true;
      }
      return false;
    }

    static bool untrack_(Processor* ptr)
    {
      if (exists_(ptr))
      {
        return procs_.erase(ptr);
      }
      return false;
    }

    public:

    static std::unordered_map<Duo, Variable>& vars() {return all_vars_;}
    static std::set<Processor*>& procs() {return Workspace::procs_;}

  };

  std::unordered_map<Duo, Variable> Workspace::all_vars_;
  template<class T> std::unordered_map<Duo, T*> Workspace::vars_;
  std::set<Processor*> Workspace::procs_;
  using ws = Workspace;
};

#endif