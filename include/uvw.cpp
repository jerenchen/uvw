#include "uvw.h"


// static "global" containers

uvw::Workspace* uvw::Workspace::curr_ = nullptr;
std::unordered_set<uvw::Workspace*> uvw::Workspace::ws_;

std::unordered_set<uvw::Processor*> uvw::Workspace::procs_;
std::unordered_map<uvw::Duohash, uvw::Duohash> uvw::Workspace::links_;
std::unordered_map<uvw::Duohash, uvw::Variable*> uvw::Workspace::vars_;
std::map<std::string, std::function<uvw::Processor*()> > uvw::Workspace::lib_;

// operator overload

bool uvw::operator==(const uvw::Duohash& lhs, const uvw::Duohash& rhs)
{
    return lhs.raw_ptr == rhs.raw_ptr && lhs.var_str == rhs.var_str;
}

// Variable impl.

void uvw::Variable::unlink()
{
  if (uvw::Workspace::links_.find(key_) != uvw::Workspace::links_.end())
  {
    uvw::Workspace::links_.erase(key_);
  }
  src_ = Duohash(nullptr,"");
  data_src_ = nullptr;
}

bool uvw::Variable::link(uvw::Variable* src)
{
  if (type_index() != src->type_index())
  {
    unlink();
    return false;
  }
  // hook up key & src data ptr
  src_ = uvw::Duohash(src->key());
  data_src_ = src->data_ptr_;
  uvw::Workspace::links_[key_] = src_;
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

uvw::Processor::Processor(const uvw::Processor& p)
{
  *this = p;
}

uvw::Processor& uvw::Processor::operator=(const uvw::Processor& p)
{
  var_keys_ = std::vector<uvw::Duohash>(p.var_keys_);
  return *this;
}

uvw::Variable* uvw::Processor::get(const std::string& label)
{
  return uvw::ws::get(uvw::Duohash(this, label));
}

// Workspace impl.

#include <queue>
#include <stack>
#include <vector>

uvw::Workspace::~Workspace()
{
  for (auto* proc_ptr : proc_ptrs_)
  {
    if (proc_ptr)
    {
      delete proc_ptr;
    }
    procs_.erase(proc_ptr);
  }
  proc_ptrs_.clear();
}
uvw::Workspace::Workspace(const Workspace& w)
{
  *this = w;
}
uvw::Workspace& uvw::Workspace::operator=(const uvw::Workspace& w)
{
  proc_ptrs_ = w.proc_ptrs_;
  return *this;
}

std::string uvw::Workspace::stats()
{
  std::string res("Stats - procs: ");
  res += std::to_string(procs_.size());
  res += " vars: ";
  res += std::to_string(vars_.size());
  res += " links: ";
  res += std::to_string(links_.size());
  return res;
}

bool uvw::Workspace::link(const uvw::Duohash& src, const uvw::Duohash& dst)
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
    uvw::Workspace::current().proc_ptrs_.push_back(proc_ptr);
    return true;
  }
  return false;
}

bool uvw::Workspace::untrack_(uvw::Processor* proc_ptr)
{
  if (uvw::Workspace::exists_(proc_ptr))
  {
    // erase-remove
    auto& proc_ptrs = uvw::Workspace::current().proc_ptrs_;
    proc_ptrs.erase(
      std::remove(proc_ptrs.begin(), proc_ptrs.end(), proc_ptr),
      proc_ptrs.end()
    );
    return uvw::Workspace::procs_.erase(proc_ptr);
  }
  return false;
}

bool uvw::Workspace::reg_proc(
  const std::string& proc_type,
  std::function<Processor*()> proc_func
)
{
  if (uvw::Workspace::lib_.find(proc_type) != uvw::Workspace::lib_.end())
  {
    return false;
  }
  uvw::Workspace::lib_[proc_type] = proc_func;
  return true;
}

uvw::Processor* uvw::Workspace::create(const std::string& proc_type)
{
  if (uvw::Workspace::lib_.find(proc_type) != uvw::Workspace::lib_.end())
  {
    uvw::Processor* proc = uvw::Workspace::lib_[proc_type]();
    proc->type_ = proc_type;
    if (proc->initialize())
    {
      return proc;
    }
    delete proc;
  }
  return nullptr;
}

std::unordered_map<uvw::Duohash, uvw::Variable*>
  uvw::Workspace::vars(uvw::Processor* proc_ptr)
{
  if (proc_ptr == nullptr)
  {
    return Workspace::vars_;
  }

  std::unordered_map<uvw::Duohash, uvw::Variable*> res;
  for (auto itr : Workspace::vars_)
  {
    if (itr.second->proc() == proc_ptr)
    {
      res[itr.first] = itr.second;
    }
  }
  return res;
}

std::unordered_set<uvw::Processor*> uvw::Workspace::procs(
  const std::string& proc_type
)
{
  if (proc_type.empty())
  {
    return Workspace::procs_;
  }

  std::unordered_set<uvw::Processor*> res;
  for (auto* ptr : Workspace::procs_)
  {
    if (ptr->type_ == proc_type)
    {
      res.insert(ptr);
    }
  }
  return res;
}

std::vector<uvw::Duohash>
  uvw::Workspace::schedule(const uvw::Duohash& key)
{
  std::vector<uvw::Duohash> res;
  if (!has(key))
  {
    return res;
  }

  std::queue<uvw::Processor*> queue_proc;
  uvw::Processor* proc = vars_[key]->proc();
  if (!proc)
  {
    std::cout << "Warning: null proc found for " << key << std::endl;
    return res;
  }
  queue_proc.push(proc);

  std::stack<uvw::Duohash> stack_vkey;
  while (queue_proc.size())
  {
    proc = queue_proc.front();
    queue_proc.pop();

    for (auto& vk : proc->var_keys_)
    {
      if (!has(vk))
      {
        continue;
      }
      Variable* var = vars_[vk];

      if (has(var->src()))
      {
        Variable* src = vars_[var->src()];
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

bool uvw::Workspace::execute(
  const std::vector<uvw::Duohash>& seq,
  bool preprocess
)
{
  std::unordered_set<void*> visited;

  uvw::Variable* var = nullptr;
  for (const auto& key : seq)
  {
    /* NOTE: a seq can only be considered valid if proc linkage 
      remains unchanged; some form of revoke mechanism is needed
      if we want to guarantee the validity of a seq */

    var = vars_[key];
    auto* proc = vars_[var->src()]->proc();
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

  if (var && exists_(var->proc()))
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

uvw::Workspace& uvw::Workspace::add()
{
  curr_ = new uvw::Workspace();
  ws_.insert(curr_);
  return *curr_;
}

uvw::Workspace& uvw::Workspace::current()
{
  if (ws_.find(curr_) == ws_.end())
  {
    return add();
  }
  return *curr_;
}

void uvw::Workspace::clear()
{
  for (auto* proc_ptr : current().proc_ptrs_)
  {
    if (proc_ptr)
    {
      delete proc_ptr;
    }
    procs_.erase(proc_ptr);
  }
  current().proc_ptrs_.clear();
}

// json

json uvw::Workspace::to_json()
{
  json data;
  if (!current().proc_ptrs_.size())
  {
    return data;
  }

  std::unordered_map<void*, unsigned int> indices_by_procs;
  data["procs"] = json::array();
  for (auto* proc_ptr : current().proc_ptrs_)
  {
    unsigned int index = indices_by_procs.size();
    indices_by_procs[proc_ptr] = index;

    data["procs"].push_back(proc_ptr->to_json());
    data["procs"].back()["index"] = index;
  }

  data["links"] = json::array();
  for (const auto& itr : links_)
  {
    if (
      indices_by_procs.find(itr.first.raw_ptr) ==
        indices_by_procs.end() &&
      indices_by_procs.find(itr.second.raw_ptr) ==
        indices_by_procs.end()
    )
    {
      // not an intra-ws link
      continue;
    }

    json link;
    link["var"]["index"] = indices_by_procs[itr.first.raw_ptr];
    link["var"]["label"] = itr.first.var_str;
    link["src"]["index"] = indices_by_procs[itr.second.raw_ptr];
    link["src"]["label"] = itr.second.var_str;
    data["links"].push_back(link);
  }

  return data;
}

bool uvw::Workspace::from_json(json& data)
{
  std::unordered_map<unsigned int, void*> procs_by_indices;

  for (auto& data_itr : data["procs"])
  {
    auto proc_type = data_itr["type"].get<std::string>();
    auto* p = create(proc_type);
    if (!p)
    {
      std::cerr << "Cannot create proc type '" <<
        proc_type << "'!" << std::endl;
      return false;
    }

    procs_by_indices[procs_by_indices.size()] = p;
    if (!p->from_json(data_itr))
    {
      return false;
    }
  }

  // intra-links amongst ws procs/vars
  for (auto& data_itr : data["links"])
  {
    if (
      procs_by_indices.find(data_itr["var"]["index"].get<int>()) ==
      procs_by_indices.end() ||
      procs_by_indices.find(data_itr["src"]["index"].get<int>()) ==
      procs_by_indices.end()
    )
    {
      continue;
    }
    auto* p = procs_by_indices[data_itr["var"]["index"].get<int>()];
    auto* q = procs_by_indices[data_itr["src"]["index"].get<int>()];
    uvw::Duohash dst(p, data_itr["var"]["label"].get<std::string>());
    uvw::Duohash src(q, data_itr["src"]["label"].get<std::string>());
    if (!link(src, dst))
    {
      std::cerr << "Cannot link between " << src << " & " << dst << std::endl;
      return false;
    }
  }
  return true;
}

json uvw::Processor::to_json()
{
  json data;

  data["type"] = type_;

  data["vars"] = json::array();
  for (auto& key : var_keys_)
  {
    if (get(key.var_str))
    {
      data["vars"].push_back(get(key.var_str)->to_json());
    }
  }
  return data;
}

bool uvw::Processor::from_json(json& data)
{
  for (auto& data_itr : data["vars"])
  {
    uvw::Variable* v = get(data_itr["label"]);
    if (!v)
    {
      std::cerr << "Cannot find var '" <<
        data_itr["label"] << "'!" << std::endl;
      return false;
    }

    v->from_json(data_itr);
  }
  return true;
}

json uvw::Variable::to_json()
{
  json data;
  data["label"] = label();
  data["typeid"] = type_index().hash_code();
  for (auto& itr : properties)
  {
    data["properties"][itr.first] = itr.second;
  }
  return data;
}

bool uvw::Variable::from_json(json& data)
{
  for (auto& itr : data["properties"].items())
  {
    properties[itr.key()] = itr.value().get<int>();
  }
  return true;
}