#include "uvw.h"


// static "global" containers

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

// Variable fundamental types; can be added to account for more types

bool uvw::Variable::data_pull = true;

std::map<std::type_index, std::string> uvw::Variable::type_strs = {
  {std::type_index(typeid(int)), "int"},
  {std::type_index(typeid(double)), "double"},
  {std::type_index(typeid(std::string)), "string"}
};

const std::string uvw::Variable::type_str()
{
  return (type_strs.find(type_index()) == type_strs.end())?
    "UNKNOWN" : type_strs[type_index()];
}

// Variable impl.

bool uvw::Variable::unlink()
{
  src_.nullify();
  data_src_ = nullptr;
  return (uvw::Workspace::links_.erase(key_) > 0);
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

uvw::Workspace::Workspace()
{
  track_(this);
}
uvw::Workspace::~Workspace()
{
  clear();
  untrack_(this);
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
    return true;
  }
  return false;
}

bool uvw::Workspace::untrack_(uvw::Processor* proc_ptr)
{
  if (uvw::Workspace::exists_(proc_ptr))
  {
    for (const auto& key : proc_ptr->var_keys())
    {
      uvw::Workspace::del(key);
    }
    return uvw::Workspace::procs_.erase(proc_ptr);
  }
  return false;
}

bool uvw::Workspace::exists_(uvw::Workspace* ws_ptr)
{
  return (
    uvw::Workspace::ws_.find(ws_ptr) !=
    uvw::Workspace::ws_.end()
  );
}

bool uvw::Workspace::track_(uvw::Workspace* ws_ptr)
{
  if (!uvw::Workspace::exists_(ws_ptr))
  {
    uvw::Workspace::ws_.insert(ws_ptr);
    return true;
  }
  return false;
}

bool uvw::Workspace::untrack_(uvw::Workspace* ws_ptr)
{
  if (uvw::Workspace::exists_(ws_ptr))
  {
    return uvw::Workspace::ws_.erase(ws_ptr);
  }
  return false;
}

void uvw::Workspace::clear()
{
  in_ = out_ = Duohash();
  seq_.clear();
  procs_by_keys_.clear();
  auto itr = proc_ptrs_.begin();
  while (itr != proc_ptrs_.end())
  {
    delete (*itr);
    itr = proc_ptrs_.erase(itr);
  }
}

bool uvw::Workspace::clear_proc_lib()
{
  // do not clear if residual proc instances exist.
  if (procs_.size())
  {
    return false;
  }
  lib_.clear();
  return true;
}

uvw::Processor* uvw::Workspace::new_proc(const std::string& proc_type)
{
  uvw::Processor* proc_ptr = uvw::Workspace::create_proc(proc_type);
  if (proc_ptr)
  {
    proc_ptrs_.push_back(proc_ptr);
    for (const auto& key : proc_ptr->var_keys())
    {
      procs_by_keys_[key] = proc_ptr;
    }
  }
  return proc_ptr;
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

bool uvw::Workspace::has_var(const uvw::Duohash& key)
{
  return (procs_by_keys_.find(key) != procs_by_keys_.end());
}

uvw::Processor* uvw::Workspace::create_proc(const std::string& proc_type)
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

std::vector<uvw::Processor*>
  uvw::Workspace::schedule(const uvw::Duohash& key)
{
  std::vector<uvw::Processor*> res;
  if (!has(key))
  {
    return res;
  }

  std::queue<uvw::Processor*> proc_queue;
  uvw::Processor* proc = vars_[key]->proc();
  if (!proc)
  {
    std::cout << "Warning: null proc found for " << key << std::endl;
    return res;
  }
  proc_queue.push(proc);

  std::stack<uvw::Processor*> proc_stack;
  proc_stack.push(proc);

  std::stack<uvw::Duohash> stack_keys;
  while (proc_queue.size())
  {
    proc = proc_queue.front();
    proc_queue.pop();

    for (const auto& var_key : proc->var_keys())
    {
      if (!has(var_key))
      {
        continue;
      }
      uvw::Variable* var = vars_[var_key];

      if (has(var->src()))
      {
        uvw::Variable* src = vars_[var->src()];
        uvw::Processor* src_proc = src->proc();
        if (src_proc)
        {
          proc_queue.push(src_proc);
          proc_stack.push(src_proc);
        }
      }
    }
  }

  std::unordered_set<void*> visited_procs;

  while (proc_stack.size())
  {
    proc = proc_stack.top();
    if (visited_procs.find(proc) == visited_procs.end())
    {
      res.push_back(proc_stack.top());
      visited_procs.insert(proc);
    }
    proc_stack.pop();
  }
  return res;
}

bool uvw::Workspace::execute(
  const std::vector<uvw::Processor*>& seq,
  bool preprocess
)
{
  for (uvw::Processor* proc_ptr : seq)
  {
    /* NOTE: a seq can only be considered valid if proc linkage 
      remains unchanged; some form of revoke mechanism is needed
      if we want to guarantee the validity of a seq */
    if (!exists_(proc_ptr))
    {
      return false;
    }

    if (uvw::Variable::data_pull)
    {
      for (auto& var_key : proc_ptr->var_keys_)
      {
        uvw::Variable* v_ = vars_[var_key];
        if (has(v_->src()))
        {
          v_->pull();
        }
      }
    }

    if (preprocess)
    {
      if (!proc_ptr->preprocess())
      {
        return false;
      }
    }

    if (!proc_ptr->process())
    {
      return false;
    }
  }

  return true;
}

bool uvw::Workspace::set_input(const Duohash& key)
{
  if (has_var(key))
  {
    in_ = key;
    return true;
  }
  return false;
}

bool uvw::Workspace::set_output(const Duohash& key)
{
  seq_.clear();
  if (has_var(key))
  {
    out_ = key;
    seq_ = uvw::Workspace::schedule(out_);
    return (seq_.size() > 0);
  }
  return false;
}

bool uvw::Workspace::process(bool preprocess)
{
  return uvw::Workspace::execute(seq_, preprocess);
}

// json

json uvw::Workspace::to_json()
{
  json data;
  if (!proc_ptrs_.size())
  {
    return data;
  }

  std::unordered_map<void*, unsigned int> indices_by_procs;
  data["procs"] = json::array();
  for (auto* proc_ptr : proc_ptrs_)
  {
    unsigned int index = indices_by_procs.size();
    indices_by_procs[proc_ptr] = index;

    data["procs"].push_back(proc_ptr->to_json());
    data["procs"].back()["index"] = index;
  }

  auto is_indexed_ = [&indices_by_procs](void* ptr)
  {
    return (indices_by_procs.find(ptr) !=
        indices_by_procs.end());
  };

  data["links"] = json::array();
  for (const auto& itr : links_)
  {
    if (!is_indexed_(itr.first.raw_ptr) && !is_indexed_(itr.second.raw_ptr))
    {
      // both end of link are not in ws scope
      continue;
    }

    json link;
    link["var"]["index"] = indices_by_procs[itr.first.raw_ptr];
    link["var"]["label"] = itr.first.var_str;
    link["src"]["index"] = indices_by_procs[itr.second.raw_ptr];
    link["src"]["label"] = itr.second.var_str;
    data["links"].push_back(link);
  }

  if (has_var(in_) && is_indexed_(in_.raw_ptr))
  {
    data["in"]["index"] = indices_by_procs[in_.raw_ptr];
    data["in"]["label"] = in_.var_str;
  }
  if (has_var(out_) && is_indexed_(out_.raw_ptr))
  {
    data["out"]["index"] = indices_by_procs[out_.raw_ptr];
    data["out"]["label"] = out_.var_str;
  }
  return data;
}

bool uvw::Workspace::from_json(json& data)
{
  std::unordered_map<unsigned int, void*> procs_by_indices;

  for (auto& data_itr : data["procs"])
  {
    auto proc_type = data_itr["type"].get<std::string>();
    auto* proc_ptr = new_proc(proc_type);
    if (!proc_ptr)
    {
      std::cerr << "Cannot create proc type '" <<
        proc_type << "'!" << std::endl;
      return false;
    }

    procs_by_indices[procs_by_indices.size()] = proc_ptr;
    if (!proc_ptr->from_json(data_itr))
    {
      return false;
    }
  }

  auto has_index_ = [&procs_by_indices](int index)
  {
    return (procs_by_indices.find(index) !=
      procs_by_indices.end());
  };

  // intra-links amongst ws procs/vars
  for (auto& data_itr : data["links"])
  {
    if (!has_index_(data_itr["var"]["index"].get<int>()) ||
          !has_index_(data_itr["src"]["index"].get<int>()))
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

  if (data.find("in") != data.end() &&
      has_index_(data["in"]["index"].get<int>()))
  {
    auto* p = procs_by_indices[data["in"]["index"].get<int>()];
    set_input(uvw::Duohash(p, data["in"]["label"].get<std::string>()));
  }
  if (data.find("out") != data.end() &&
      has_index_(data["out"]["index"].get<int>()))
  {
    auto* q = procs_by_indices[data["out"]["index"].get<int>()];
    set_output(uvw::Duohash(q, data["out"]["label"].get<std::string>()));
  }
  return true;
}

json uvw::Processor::to_json()
{
  json data;

  data["type"] = type_;

  data["vars"] = json::array();
  for (const auto& key : var_keys())
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
  data["type"] = type_str();
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