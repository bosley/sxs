
/*
  Sometiems we need a shared pointer, but we don't want the size of the shared
  pointer. This implementation is smaller than std::shared_pointer, and not as
  "safe to use".. technically
*/

#pragma once

#include <cstdint>

namespace pkg::types {

//! \brief Something that will be reference counted
class shared_c {
public:
  shared_c() {}
  virtual ~shared_c() {}

  const shared_c *acquire() const {
    ref_count_++;
    return this;
  }
  int64_t release() const { return --ref_count_; }
  int64_t refCount() const { return ref_count_; }

private:
  shared_c(const shared_c &);
  shared_c &operator=(const shared_c &);

  mutable std::uint32_t ref_count_{0};
};

//! \brief A wrapper that performs operations
//!        for and around a reference counted object
template <class T> class shared_obj_c {
public:
  shared_obj_c() {}

  shared_obj_c(T *object) { acquire(object); }

  shared_obj_c(const shared_obj_c &rhs) : object_(0) { acquire(rhs.object_); }

  const shared_obj_c &operator=(const shared_obj_c &rhs) {
    if (this != &rhs) {
      acquire(rhs.object_);
    }
    return *this;
  }

  T &operator*() const { return *object_; }

  T *get() const { return object_; }

  bool operator==(const shared_obj_c &rhs) const {
    return object_ == rhs.object_;
  }

  bool operator!=(const shared_obj_c &rhs) const {
    return object_ != rhs.object_;
  }

  operator bool() const { return object_ != nullptr; }

  ~shared_obj_c() { release(); }

  T *operator->() const { return object_; }
  T *ptr() const { return object_; }

private:
  void acquire(T *object) {
    if (object != nullptr) {
      object->acquire();
    }
    release();
    object_ = object;
  }

  void release() {
    if ((object_ != nullptr) && (object_->release() == 0)) {
      delete object_;
      object_ = nullptr;
    }
  }

  T *object_{nullptr};
};

template <typename T>
auto make_shared =
    [](auto... args) -> shared_obj_c<T> { return new T(args...); };

} // namespace pkg::types