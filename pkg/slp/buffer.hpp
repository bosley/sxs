#pragma once

#include <cstddef>
#include <cstdint>

namespace slp {

class slp_buffer_c {
public:
  slp_buffer_c();
  ~slp_buffer_c();

  slp_buffer_c(const slp_buffer_c &other);
  slp_buffer_c &operator=(const slp_buffer_c &other);

  slp_buffer_c(slp_buffer_c &&other) noexcept;
  slp_buffer_c &operator=(slp_buffer_c &&other) noexcept;

  std::uint8_t *data();
  const std::uint8_t *data() const;

  std::size_t size() const;
  std::size_t capacity() const;
  bool empty() const;

  void resize(std::size_t new_size);
  void reserve(std::size_t new_capacity);
  void clear();

  std::uint8_t &operator[](std::size_t index);
  const std::uint8_t &operator[](std::size_t index) const;

  bool operator==(const slp_buffer_c &other) const;
  bool operator!=(const slp_buffer_c &other) const;

  void insert(std::size_t pos, const std::uint8_t *data, std::size_t count);

private:
  std::uint8_t *data_;
  std::size_t size_;
  std::size_t capacity_;

  void grow_to(std::size_t min_capacity);
  void free_data();
};

} // namespace slp
