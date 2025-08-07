#pragma once

#include <concepts>
#include <limits>

namespace pkg::types {

template<typename T>
concept arithmetic = std::integral<T> or std::floating_point<T>;

// Monotonic counter that will increase or decrease by given
// value until it hits the maximum range of the given type
//  - For a monotonic decreasing function, set the INC_VAL to
//    a negative number
template<arithmetic T, T INC_VAL>
class monotonic_counter_c {
public:
  monotonic_counter_c(const T& default_value) 
    : _value(default_value) {}

  const T get() const { return _value; }

  const T next() {
    T current = _value;
    inc();
    return current;
  }

  monotonic_counter_c& operator++() {
    inc();
    return *this;
  }
private:
  T _value;

  inline void inc() {
    if constexpr(INC_VAL < 0) {
      if (_value == std::numeric_limits<T>::min()){ return; }
    }
    if constexpr(INC_VAL >= 0) {
      if (_value == std::numeric_limits<T>::max()) { return; }
    }
    _value += INC_VAL;
  }
};

} // namespace pkg::types
