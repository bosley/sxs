#include "slp/buffer.hpp"
#include <algorithm>
#include <cstring>

namespace slp {

slp_buffer_c::slp_buffer_c() : data_(nullptr), size_(0), capacity_(0) {}

slp_buffer_c::~slp_buffer_c() { free_data(); }

slp_buffer_c::slp_buffer_c(const slp_buffer_c &other)
    : data_(nullptr), size_(0), capacity_(0) {
  if (other.size_ > 0) {
    reserve(other.size_);
    std::memcpy(data_, other.data_, other.size_);
    size_ = other.size_;
  }
}

slp_buffer_c &slp_buffer_c::operator=(const slp_buffer_c &other) {
  if (this != &other) {
    if (other.size_ > capacity_) {
      free_data();
      reserve(other.size_);
    }
    if (other.size_ > 0) {
      std::memcpy(data_, other.data_, other.size_);
    }
    size_ = other.size_;
  }
  return *this;
}

slp_buffer_c::slp_buffer_c(slp_buffer_c &&other) noexcept
    : data_(other.data_), size_(other.size_), capacity_(other.capacity_) {
  other.data_ = nullptr;
  other.size_ = 0;
  other.capacity_ = 0;
}

slp_buffer_c &slp_buffer_c::operator=(slp_buffer_c &&other) noexcept {
  if (this != &other) {
    free_data();
    data_ = other.data_;
    size_ = other.size_;
    capacity_ = other.capacity_;
    other.data_ = nullptr;
    other.size_ = 0;
    other.capacity_ = 0;
  }
  return *this;
}

std::uint8_t *slp_buffer_c::data() { return data_; }

const std::uint8_t *slp_buffer_c::data() const { return data_; }

std::size_t slp_buffer_c::size() const { return size_; }

std::size_t slp_buffer_c::capacity() const { return capacity_; }

bool slp_buffer_c::empty() const { return size_ == 0; }

void slp_buffer_c::resize(std::size_t new_size) {
  if (new_size > capacity_) {
    grow_to(new_size);
  }
  if (new_size > size_ && data_ != nullptr) {
    std::memset(data_ + size_, 0, new_size - size_);
  }
  size_ = new_size;
}

void slp_buffer_c::reserve(std::size_t new_capacity) {
  if (new_capacity > capacity_) {
    grow_to(new_capacity);
  }
}

void slp_buffer_c::clear() { size_ = 0; }

std::uint8_t &slp_buffer_c::operator[](std::size_t index) {
  return data_[index];
}

const std::uint8_t &slp_buffer_c::operator[](std::size_t index) const {
  return data_[index];
}

bool slp_buffer_c::operator==(const slp_buffer_c &other) const {
  if (size_ != other.size_) {
    return false;
  }
  if (size_ == 0) {
    return true;
  }
  return std::memcmp(data_, other.data_, size_) == 0;
}

bool slp_buffer_c::operator!=(const slp_buffer_c &other) const {
  return !(*this == other);
}

void slp_buffer_c::insert(std::size_t pos, const std::uint8_t *data,
                          std::size_t count) {
  if (count == 0) {
    return;
  }

  std::size_t new_size = size_ + count;
  if (new_size > capacity_) {
    grow_to(new_size);
  }

  if (pos < size_) {
    std::memmove(data_ + pos + count, data_ + pos, size_ - pos);
  }

  std::memcpy(data_ + pos, data, count);
  size_ = new_size;
}

void slp_buffer_c::grow_to(std::size_t min_capacity) {
  std::size_t new_capacity = capacity_;

  if (new_capacity == 0) {
    new_capacity = 16;
  }

  while (new_capacity < min_capacity) {
    new_capacity *= 2;
  }

  std::uint8_t *new_data = new std::uint8_t[new_capacity];

  if (data_ != nullptr && size_ > 0) {
    std::memcpy(new_data, data_, size_);
  }

  free_data();
  data_ = new_data;
  capacity_ = new_capacity;
}

void slp_buffer_c::free_data() {
  if (data_ != nullptr) {
    delete[] data_;
    data_ = nullptr;
  }
  capacity_ = 0;
}

} // namespace slp
