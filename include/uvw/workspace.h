#ifndef UVW_WORKSPACE_H
#define UVW_WORKSPACE_H

#include "duohash.h"
#include "variable.h"

#include <unordered_map>
#include <set>
#include <map>
#include <vector>
#include <functional>
#include <iostream>


namespace uvw
{
  class Processor;

  class Workspace
  {
    friend class Variable;
    friend class Processor;

    protected:

    static std::unordered_map<Duohash, Variable*> vars_;
    static std::unordered_map<Duohash, Duohash> links_;

    public:

    static bool has(const Duohash& key)
    {
      return (vars_.find(key) != vars_.end());
    }

    static bool del(const Duohash& key)
    {
      if (has(key))
      {
        vars_.erase(key);
        return true;
      }
      return false;
    }

    static Variable* get(const Duohash& key)
    {
      return has(key)? vars_[key]:nullptr;
    }

    template<typename T>
    static bool add(const Duohash& key, Var<T>& v)
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
    static T& ref(const Duohash& key)
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

    static bool link(const Duohash& src, const Duohash& dst);

    static std::vector<Duohash> schedule(const Duohash& key);
    static bool execute(const std::vector<Duohash>& seq, bool preprocess = false);

    // vars/links/procs range-based

    static std::set<Processor*> procs(
      const std::string& proc_type = std::string()
    );
    static std::unordered_map<Duohash, Variable*> vars(
      Processor* proc = nullptr
    );
    static std::unordered_map<Duohash, Duohash>& links() {return links_;}

    protected:
    // TODO: unordered map
    static std::set<Processor*> procs_;

    static bool exists_(Processor* proc_ptr);
    static bool track_(Processor* proc_ptr);
    static bool untrack_(Processor* proc_ptr);

    public:
    // proc funcs
    static void clear();
    static std::string stats();

    // proc json serialization
    static json to_json();
    static bool from_json(json& data);

    // proc library registeration

    protected:

    static std::map<std::string, std::function<Processor*()> > lib_;

    public:

    static bool reg_proc(
      const std::string& proc_type,
      std::function<Processor*()> proc_func
    );
    static Processor* create(const std::string& proc_type);
  };

  using ws = Workspace;
};

#endif