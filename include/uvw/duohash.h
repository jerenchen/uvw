#ifndef UVW_DUOHASH_H
#define UVW_DUOHASH_H

#include <iostream>


namespace uvw
{
  struct Duo
  {
    void* raw_ptr;
    std::string var_str;

    public:

    Duo(void* proc = nullptr, const std::string& label = std::string()):
      raw_ptr(proc), var_str(label) {}
    Duo(const Duo& key) {this->raw_ptr = key.raw_ptr; this->var_str = key.var_str;}

    friend std::ostream& operator<<(std::ostream& os, const Duo& key)
    {
      return os << "[" << key.var_str << "]@" << key.raw_ptr;
    }

    const bool is_null() const {return raw_ptr == nullptr || var_str.empty();}
  };

  bool operator==(const Duo& lhs, const Duo& rhs);
};

namespace std
{
  template<> struct
  hash<uvw::Duo>
  {
    std::size_t operator()(uvw::Duo const& duo) const noexcept
    {
        return std::hash<void*>{}(duo.raw_ptr) ^
          (std::hash<std::string>{}(duo.var_str) << 1);
    }
  };
}

#endif