#ifndef UVW_WORKSPACE_H
#define UVW_WORKSPACE_H

#include "duohash.h"
#include "variable.h"

#include <unordered_set>
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
        vars_[key]->unlink();
        return (vars_.erase(key) > 0);
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

    static std::vector<Processor*> schedule(const Duohash& key);
    static bool execute(const std::vector<Processor*>& seq, bool preprocess = false);

    static std::string stats();

    protected:

    static std::unordered_set<Workspace*> ws_;
    static bool exists_(Workspace* ws_ptr);
    static bool track_(Workspace* ws_ptr);
    static bool untrack_(Workspace* ws_ptr);

    static std::unordered_set<Processor*> procs_;
    static bool exists_(Processor* proc_ptr);
    static bool track_(Processor* proc_ptr);
    static bool untrack_(Processor* proc_ptr);

    // workspace instance vars/funcs
    public:

    void clear();
    Processor* new_proc(const std::string& proc_type);

    bool has_var(const Duohash& key);
    const std::vector<Processor*>& proc_ptrs() const {return proc_ptrs_;}

    // proc json serialization
    json to_json();
    bool from_json(json& data);

    // workspace processing
    bool set_input(const Duohash& key);
    bool set_output(const Duohash& key);
    bool process(bool preprocess = false);

    protected:

    // per-workspace proc container
    std::vector<Processor*> proc_ptrs_;
    std::unordered_map<Duohash, Processor*> procs_by_keys_;

    Duohash in_, out_;
    std::vector<Processor*> seq_;

    public:

    Workspace();
    ~Workspace();
    Workspace(const Workspace& w);
    Workspace& operator=(const Workspace& w);

    // vars/links/procs range-based

    static std::unordered_set<Processor*> procs(
      const std::string& proc_type = std::string()
    );
    static std::unordered_map<Duohash, Variable*> vars(
      Processor* proc = nullptr
    );
    static std::unordered_map<Duohash, Duohash>& links() {return links_;}
    static std::unordered_set<Workspace*>& workspaces() {return ws_;}

    // proc library registeration

    protected:

    static std::map<std::string, std::function<Processor*()> > lib_;

    public:

    static bool clear_proc_lib();
    static bool reg_proc(
      const std::string& proc_type,
      std::function<Processor*()> proc_func
    );
    static Processor* create_proc(const std::string& proc_type);
  };

  using ws = Workspace;
};

#endif