#ifndef UVW_WORKSPACE_H
#define UVW_WORKSPACE_H

#include <unordered_map>
#include <set>
#include <iostream>

#include "variable.h"

namespace uvw
{
  class Processor;

  class Workspace
  {
    friend class Processor;

    protected:

    static std::unordered_map<std::string, Variable> all_vars_;
    template<class T> static std::unordered_map<std::string, T> vars_;

    public:
    
    static bool has_var(const std::string& var)
    {
      return (all_vars_.find(var) != all_vars_.end());
    }

    template<typename T>
    static bool has(const std::string& var)
    {
      return (vars_<T>.find(var) != vars_<T>.end());
    }

    template<typename T>
    static bool del(const std::string& var)
    {
      if (vars_<T>.find(var) == vars_<T>.end())
      {
        return false;
      }

      vars_<T>.erase(var);

      if (has_var(var))
      {
        all_vars_.erase(var);
      }

      return true;
    }

    template<typename T>
    static T& get(const std::string& var)
    {
      return has<T>(var)? vars_<T>[var] : Variable::null_<T>;
    }

    template<typename T>
    static bool add(const std::string& var)
    {
      if (has_var(var))
      {
        return false;
      }

      vars_<T>[var] = T();
      all_vars_[var] = Variable(var, Variable::var_type<T>);

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
  };

  std::unordered_map<std::string, Variable> Workspace::all_vars_;
  template<class T> std::unordered_map<std::string, T> Workspace::vars_;
  std::set<Processor*> Workspace::procs_;
  using ws = Workspace;
};

#endif