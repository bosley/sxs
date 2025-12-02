#pragma once

#include <array>
#include <cstddef>
#include <functional>
#include <mutex>
#include <stdexcept>

namespace pkg::cache {

template <typename T, std::size_t N> class cache_c {
public:
  template <std::size_t M> class range_c {
    static_assert(M <= N, "Range size cannot exceed cache size");

  public:
    range_c(cache_c &cache, std::size_t start_idx) : cache_(cache), index_(0) {
      std::lock_guard<std::mutex> lock(cache_.mutex_);
      if (start_idx + M > N) {
        throw std::out_of_range("Range would exceed cache bounds");
      }
      for (std::size_t i = 0; i < M; ++i) {
        data_[i] = &cache_.data_[start_idx + i];
      }
    }

    ~range_c() = default;

    T &operator[](std::size_t idx) {
      std::lock_guard<std::mutex> lock(cache_.mutex_);
      if (idx >= M) {
        throw std::out_of_range("Index out of range bounds");
      }
      return *data_[idx];
    }

    const T &operator[](std::size_t idx) const {
      std::lock_guard<std::mutex> lock(cache_.mutex_);
      if (idx >= M) {
        throw std::out_of_range("Index out of range bounds");
      }
      return *data_[idx];
    }

    constexpr std::size_t size() const { return M; }

    T **begin() { return data_.data(); }
    T **end() { return data_.data() + M; }
    const T *const *begin() const { return data_.data(); }
    const T *const *end() const { return data_.data() + M; }

    bool operator==(const range_c &other) const {
      return data_ == other.data_ && index_ == other.index_;
    }

  private:
    cache_c &cache_;
    std::array<T *, M> data_;
    std::size_t index_;
  };

  cache_c(const T &default_value) : default_value_(default_value) {
    data_.fill(default_value_);
  }

  ~cache_c() = default;

  cache_c(const cache_c &) = delete;
  cache_c &operator=(const cache_c &) = delete;
  cache_c(cache_c &&) = delete;
  cache_c &operator=(cache_c &&) = delete;

  T &operator[](std::size_t idx) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (idx >= N) {
      throw std::out_of_range("Index out of cache bounds");
    }
    return data_[idx];
  }

  const T &operator[](std::size_t idx) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (idx >= N) {
      throw std::out_of_range("Index out of cache bounds");
    }
    return data_[idx];
  }

  template <std::size_t M> range_c<M> range(std::size_t start_idx = 0) {
    return range_c<M>(*this, start_idx);
  }

  constexpr std::size_t size() const { return N; }

  void with_lock(std::function<void(std::array<T, N> &)> func) {
    std::lock_guard<std::mutex> lock(mutex_);
    func(data_);
  }

  void with_lock(std::function<void(const std::array<T, N> &)> func) const {
    std::lock_guard<std::mutex> lock(mutex_);
    func(data_);
  }

  T *begin() { return data_.data(); }
  T *end() { return data_.data() + N; }
  const T *begin() const { return data_.data(); }
  const T *end() const { return data_.data() + N; }

private:
  T default_value_;
  std::array<T, N> data_;
  mutable std::mutex mutex_;

  template <std::size_t M> friend class range_c;
};

} // namespace pkg::cache
