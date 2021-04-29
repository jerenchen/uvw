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
  {std::type_index(typeid(int64_t)), "int64"},
  {std::type_index(typeid(bool)), "bool"},
  {std::type_index(typeid(double)), "double"},
  {std::type_index(typeid(std::string)), "string"}
};

const std::string uvw::Variable::type_str()
{
  return (type_strs.find(type_index()) == type_strs.end())?
    "UNKNOWN" : type_strs[type_index()];
}

// Variable impl.

#include <queue>

void uvw::Variable::propagate(void* data_src)
{
  std::queue<uvw::Duohash> key_queue;
  key_queue.push(key_);
  while (key_queue.size())
  {
    auto* v_ = uvw::Workspace::get(key_queue.front());
    key_queue.pop();
    if (v_)
    {
      v_->data_src_ = data_src;
      for (auto k : v_->incoming_)
      {
        key_queue.push(k);
      }
    }
  }
}

bool uvw::Variable::unlink()
{
  if (uvw::Workspace::has(src_))
  {
    uvw::Workspace::get(src_)->incoming_.erase(key_);
  }

  src_.nullify();
  // ensure to point downstream to self
  propagate(data_ptr_);
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

  // populate downstream/incoming links
  propagate(src->data_ptr_);

  src->incoming_.insert(key_);
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
  res += " ws: ";
  res += std::to_string(ws_.size());
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
  auto itr = proc_ptrs_.begin();
  while (itr != proc_ptrs_.end())
  {
    delete (*itr);
    itr = proc_ptrs_.erase(itr);
  }
  seq_.clear();
  procs_by_keys_.clear();
  in_ = Duohash();
  out_ = Duohash();
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

    if (!proc_ptr->process(preprocess))
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

bool uvw::Workspace::from_str(const std::string& str)
{
  json data;
  std::string err = picojson::parse(data, str);
  if (!err.empty())
  {
    std::cerr << err << std::endl;
    return false;
  }
  return from_json(data);
}

json uvw::Workspace::to_json()
{
  if (!proc_ptrs_.size())
  {
    return json();
  }

  json::object data_obj;
  std::unordered_map<void*, int64_t> indices_by_procs;
  json::array proc_list;
  for (auto* proc_ptr : proc_ptrs_)
  {
    int64_t index = indices_by_procs.size();
    indices_by_procs[proc_ptr] = index;

    auto proc_obj = proc_ptr->to_json().get<json::object>();
    proc_obj["index"] = json(index);
    proc_list.push_back(json(proc_obj));
  }
  data_obj["procs"] = json(proc_list);

  auto is_indexed_ = [&indices_by_procs](void* ptr)
  {
    return (indices_by_procs.find(ptr) != indices_by_procs.end());
  };

  json::array link_list;
  for (const auto& itr : links_)
  {
    if (is_indexed_(itr.first.raw_ptr) || is_indexed_(itr.second.raw_ptr))
    {
      json::object var_obj;
      json::object src_obj;
      var_obj["index"] = json(indices_by_procs[itr.first.raw_ptr]);
      var_obj["label"] = json(itr.first.var_str);
      src_obj["index"] = json(indices_by_procs[itr.second.raw_ptr]);
      src_obj["label"] = json(itr.second.var_str);
      json::object link_obj;
      link_obj["var"] = json(var_obj);
      link_obj["src"] = json(src_obj);
      link_list.push_back(json(link_obj));
    }
    else
    {
      // neither is in the ws scope...
    }
  }
  data_obj["links"] = json(link_list);

  if (has_var(in_) && is_indexed_(in_.raw_ptr))
  {
    json::object in_obj;
    in_obj["index"] = json(indices_by_procs[in_.raw_ptr]);
    in_obj["label"] = json(in_.var_str);
    data_obj["in"] = json(in_obj);
  }
  if (has_var(out_) && is_indexed_(out_.raw_ptr))
  {
    json::object out_obj;
    out_obj["index"] = json(indices_by_procs[out_.raw_ptr]);
    out_obj["label"] = json(out_.var_str);
    data_obj["out"] = json(out_obj);
  }
  return json(data_obj);
}

bool uvw::Workspace::from_json(json& data)
{
  auto& data_obj = data.get<json::object>();
  std::unordered_map<int64_t, void*> procs_by_indices;

  if (data_obj.find("procs") != data_obj.end() &&
        data_obj["procs"].is<json::array>())
  {
    for (auto& data_itr : data_obj["procs"].get<json::array>())
    {
      auto& proc_obj = data_itr.get<json::object>();
      auto proc_type = proc_obj["type"].get<std::string>();
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
  }

  auto has_index_ = [&procs_by_indices](int64_t index)
  {
    return (procs_by_indices.find(index) !=
      procs_by_indices.end());
  };

  // intra-links amongst ws procs/vars
  if (data_obj.find("links") != data_obj.end() &&
        data_obj["links"].is<json::array>())
  {
    for (auto& data_itr : data_obj["links"].get<json::array>())
    {
      auto& link_obj = data_itr.get<json::object>();
      auto& var_obj = link_obj["var"].get<json::object>();
      auto& src_obj = link_obj["src"].get<json::object>();
      auto var_index = var_obj["index"].get<int64_t>();
      auto src_index = src_obj["index"].get<int64_t>();
      if (!has_index_(var_index) || !has_index_(src_index))
      {
        std::cout << "Cannot find proc index " << var_index <<
          " or " << src_index << "! Skipping..." << std::endl;
        continue;
      }
      auto* p = procs_by_indices[var_index];
      auto* q = procs_by_indices[src_index];
      uvw::Duohash dst(p, var_obj["label"].get<std::string>());
      uvw::Duohash src(q, src_obj["label"].get<std::string>());
      if (!link(src, dst))
      {
        std::cerr << "Cannot link between " << src << " & " << dst << std::endl;
        return false;
      }
    }
  }

  if (data_obj.find("in") != data_obj.end())
  {
    auto& in_obj = data_obj["in"].get<json::object>();
    auto in_index = in_obj["index"].get<int64_t>();
    if (has_index_(in_index))
    {
      auto* p = procs_by_indices[in_index];
      auto k_in = uvw::Duohash(p, in_obj["label"].get<std::string>());
      if (!set_input(k_in))
      {
        std::cerr << "Cannot set input " << k_in << "!" << std::endl;
      }
    }
    else
    {
      std::cerr << "Cannot find in index " << in_index << "!" << std::endl;
    }
  }
  if (data_obj.find("out") != data_obj.end())
  {
    auto& out_obj = data_obj["out"].get<json::object>();
    auto out_index = out_obj["index"].get<int64_t>();
    if (has_index_(out_index))
    {
      auto* q = procs_by_indices[out_index];
      auto k_out = uvw::Duohash(q, out_obj["label"].get<std::string>());
      if (!set_output(k_out))
      {
        std::cerr << "Cannot set output " << k_out << "!" << std::endl;
      }
    }
    else
    {
      std::cerr << "Cannot find out index " << out_index << "!" << std::endl;
    }
  }
  return true;
}

json uvw::Processor::to_json()
{
  json::object data_obj;
  data_obj["type"] = json(type_);
  json::array var_list;
  for (const auto& key : var_keys())
  {
    if (get(key.var_str))
    {
      var_list.push_back(json(get(key.var_str)->to_json()));
    }
  }
  data_obj["vars"] = json(var_list);
  return json(data_obj);
}

bool uvw::Processor::from_json(json& data)
{
  auto& data_obj = data.get<json::object>();
  if (data_obj.find("vars") != data_obj.end())
  {
    for (auto& data_itr : data_obj["vars"].get<json::array>())
    {
      auto& var_obj = data_itr.get<json::object>();
      auto var_label = var_obj["label"].get<std::string>();
      uvw::Variable* v = get(var_label);
      if (!v)
      {
        std::cerr << "Cannot find var '" << var_label << "'!" << std::endl;
        return false;
      }
      v->from_json(data_itr);
    }
  }
  return true;
}

json uvw::Variable::to_json()
{
  json::object data_obj;
  data_obj["enabled"] = json(enabled);
  data_obj["label"] = json(label());
  data_obj["type"] = json(type_str());
  if (properties.size())
  {
    json::object prop_obj;
    for (auto& itr : properties)
    {
      prop_obj[itr.first] = json((int64_t)itr.second);
    }
    data_obj["properties"] = json(prop_obj);
  }
  return json(data_obj);
}

bool uvw::Variable::from_json(json& data)
{
  auto& data_obj = data.get<json::object>();
  properties.clear();
  if (data_obj.find("properties") != data_obj.end())
  {
    for (auto& itr : data_obj["properties"].get<json::object>())
    {
      properties[itr.first] = itr.second.get<int64_t>();
    }
  }
  if (data_obj.find("enabled") != data_obj.end())
  {
    enabled = data_obj["enabled"].get<bool>();
  }
  return true;
}