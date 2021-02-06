
#include "uvw.h"


// operator overload

bool uvw::operator==(const Duo& lhs, const Duo& rhs)
{
    return lhs.raw_ptr == rhs.raw_ptr && lhs.var_str == rhs.var_str;
}

// static "global" containers

std::unordered_map<uvw::Duo, uvw::Variable*> uvw::Workspace::vars_;
std::set<uvw::Processor*> uvw::Workspace::procs_;

// Variable impl.

bool uvw::Variable::link(uvw::Variable* src)
{
  if ((type_ != 0) && (type_ != src->type_))
  {
    unlink();
    return false;
  }
  // hook up key & src data ptr
  src_ = uvw::Duo(src->key());
  data_src_ = src->data_ptr_;
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

// Processor impl.

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

uvw::Variable* uvw::Processor::get(const std::string& label)
{
  return uvw::ws::get(uvw::Duo(this, label));
}

// Workspace impl.

#include <queue>
#include <stack>
#include <vector>

bool uvw::Workspace::link(const uvw::Duo& src, const uvw::Duo& dst)
{
  if (!uvw::Workspace::has(src) || !uvw::Workspace::has(dst))
  {
    return false;
  }
  return uvw::Workspace::vars_[dst]->link(uvw::Workspace::vars_[src]);
}

bool uvw::Workspace::exists_(uvw::Processor* proc_ptr)
{
  return (
    uvw::Workspace::procs_.find(proc_ptr) !=
    uvw::Workspace::procs_.end()
  );
}

bool uvw::Workspace::track_(uvw::Processor* proc_ptr)
{
  if (!uvw::Workspace::exists_(proc_ptr))
  {
    uvw::Workspace::procs_.insert(proc_ptr);
    return true;
  }
  return false;
}

bool uvw::Workspace::untrack_(uvw::Processor* proc_ptr)
{
  if (uvw::Workspace::exists_(proc_ptr))
  {
    return uvw::Workspace::procs_.erase(proc_ptr);
  }
  return false;
}

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

bool uvw::Workspace::execute(const std::vector<uvw::Duo>& seq, bool preprocess)
{
  std::unordered_set<void*> visited;

  uvw::Variable* var = nullptr;
  for (const auto& key : seq)
  {
    // Assuming var & its src are validated by schedule...
    var = uvw::Workspace::vars_[key];
    auto* proc = uvw::Workspace::vars_[var->src()]->proc();
    if (visited.find(proc) == visited.end())
    {
      if (preprocess)
      {
        if (!proc->preprocess())
        {
          return false;
        }
      }

      if (!proc->process())
      {
        return false;
      }

      visited.insert(proc);
    }
    // var is ready to pull
    var->pull();
  }

  if (var && uvw::Workspace::exists_(var->proc()))
  {
    auto* proc = var->proc();
    if (visited.find(proc) == visited.end())
    {
      if (preprocess)
      {
        if (!proc->preprocess())
        {
          return false;
        }
      }
      return proc->process();
    }
  }

  return true;
}