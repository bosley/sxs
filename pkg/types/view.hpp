#pragma once

#include <cstddef>
#include <memory>

namespace pkg::types {

template <typename T> class view_c {
public:
  class iter_c {
  public:
    virtual ~iter_c() = default;
    virtual bool has_next() const = 0;
    virtual T *next() = 0;
    virtual void reset() = 0;
  };
  using iter_ptr = std::shared_ptr<iter_c>;

  view_c() = default;

  view_c(T *target, const std::size_t len) : target_(target), len_(len) {}

  const bool empty() const { return target_ == nullptr; }
  const std::size_t size() const { return len_; }

  T *operator[](const std::size_t idx) {
    if (!target_ || idx >= len_) {
      return nullptr;
    }
    return (T *)(target_ + idx);
  };

  //! \brief Obtain an iterator to the view
  //! \note  The user must ensure that the iterator does not
  //!        outlive the view lest they experience UB
  iter_ptr iter() { return std::make_shared<iter_s>(this); }

private:
  class iter_s final : public iter_c {
  public:
    iter_s(view_c<T> *ptrin) : ptr(ptrin) {}
    bool has_next() const override { return idx < ptr->size(); }
    T *next() override {
      if (!has_next())
        return nullptr;
      return (*ptr)[idx++];
    }
    void reset() override { idx = 0; }
    view_c<T> *ptr{nullptr};
    std::size_t idx{0};
  };
  T *target_{nullptr};
  const std::size_t len_{0};
};

} // namespace pkg::types