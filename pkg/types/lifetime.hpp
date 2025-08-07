/*

  Lifetime objects that provide an observer that is
  called upon the termination of the lifetime

  - lifetime_c : On death, simple notification 
  - lifetime_tagged_c : On death, a supplied tag is given
      to discriminate the object that has indicated death
*/

#pragma once

#include <cstddef>

namespace pkg::types {

class lifetime_c {
public:
  class observer_if {
  public:
    virtual void death_ind() = 0;
  };
  lifetime_c() = delete;
  constexpr lifetime_c(observer_if &obs) : obs_(obs){}
  ~lifetime_c() {
    obs_.death_ind();
  }
private:
  observer_if& obs_;
};

class lifetime_tagged_c {
public:
  class observer_if {
  public:
    virtual void death_ind(const std::size_t tag) = 0;
  };
  lifetime_tagged_c() = delete;
  constexpr lifetime_tagged_c(
      observer_if &obs, const std::size_t tag)
    : obs_(obs), tag_(tag){}
  ~lifetime_tagged_c() {
    obs_.death_ind(tag_);
  }
private:
  observer_if& obs_;
  const std::size_t tag_{0};
};

} // namespace pkg::types
