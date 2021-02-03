#ifndef UVW_DUOHASH_H
#define UVW_DUOHASH_H

#include <iostream>


namespace uvw
{
  struct Duo
  {
    void* ptr;
    std::string var;

    Duo(void* proc = nullptr, const std::string& label = std::string()):
      ptr(proc), var(label) {}
    Duo(const Duo& key) {ptr = key.ptr; var = key.var;}

    bool is_null() {return ptr == nullptr || var.empty();}
  };
  bool operator==(const Duo& lhs, const Duo& rhs)
  {
      return lhs.ptr == rhs.ptr && lhs.var == rhs.var;
  }
};

namespace std
{
  template<> struct
  hash<uvw::Duo>
  {
    std::size_t operator()(uvw::Duo const& duo) const noexcept
    {
        return std::hash<void*>{}(duo.ptr) ^
          (std::hash<std::string>{}(duo.var) << 1); 
    }
  };
}

#endif