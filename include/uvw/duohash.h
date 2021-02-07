#ifndef UVW_DUOHASH_H
#define UVW_DUOHASH_H

#include <functional>

#include <iostream>


namespace uvw
{
  struct Duohash
  {
    void* raw_ptr;
    std::string var_str;

    public:

    Duohash(void* proc = nullptr, const std::string& label = std::string()):
      raw_ptr(proc), var_str(label) {}
    Duohash(const Duohash& key)
    {
      this->raw_ptr = key.raw_ptr;
      this->var_str = key.var_str;
    }

    friend std::ostream& operator<<(std::ostream& os, const Duohash& key)
    {
      return os << "[" << key.var_str << "]@" << key.raw_ptr;
    }

    const bool is_null() const {return raw_ptr == nullptr || var_str.empty();}
  };

  bool operator==(const Duohash& lhs, const Duohash& rhs);

  using duo = Duohash;
};

namespace std
{
  template<> struct
  hash<uvw::Duohash>
  {
    std::size_t operator()(uvw::Duohash const& key) const noexcept
    {
        return std::hash<void*>{}(key.raw_ptr) ^
          (std::hash<std::string>{}(key.var_str) << 1);
    }
  };
}

#endif