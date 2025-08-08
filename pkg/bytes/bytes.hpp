#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

namespace pkg::bytes {

//! \brief Merge two vectors together
static constexpr auto merge_vecs = [](std::vector<uint8_t> &to,
                                      const std::vector<uint8_t> &from) {
  to.insert(to.end(), from.begin(), from.end());
};

//! \brief Pack some type T to a vector of bytes
template <typename T, typename = typename std::enable_if<
                          std::is_arithmetic<T>::value, T>::type>
static inline std::vector<uint8_t> pack(const T value) {
  return std::vector<uint8_t>((uint8_t *)&value,
                              (uint8_t *)&(value) + sizeof(T));
}

//! \brief Directly pack some type T to a vector of bytes
template <typename T, typename = typename std::enable_if<
                          std::is_arithmetic<T>::value, T>::type>
static inline void pack_into(const T value, std::vector<uint8_t> &target) {
  target.insert(target.end(), (uint8_t *)&value,
                (uint8_t *)&(value) + sizeof(T));
}

//! \brief Unpack some vector of bytes to type T
template <typename T, typename = typename std::enable_if<
                          std::is_arithmetic<T>::value, T>::type>
static inline std::optional<T> unpack(const std::vector<uint8_t> &data) {
  if (data.size() != sizeof(T)) {
    return std::nullopt;
  }
  T *val = (T *)(data.data());
  return {*val};
}

//! \brief Attempt to quickly unpack a vector of bytes into type T
//! \note  This is potentially unsafe (check that the vector is a valid T)
template <typename T, typename = typename std::enable_if<
                          std::is_arithmetic<T>::value, T>::type>
static inline T quick_unpack(const std::vector<uint8_t> &data) {
  T x = *((T *)(data.data()));
  return x;
}

//! \brief Convert string to a vector of bytes
static inline std::vector<uint8_t> pack_string(const std::string &str) {
  return std::vector<uint8_t>(str.begin(), str.end());
}

//! \brief Pack a string, and its length into a vector
static inline void pack_string_into(const std::string &str,
                                    std::vector<uint8_t> &target) {
  pack_into<std::size_t>(str.size(), target);
  target.insert(target.end(), str.begin(), str.end());
}

template <class T>
static inline void pack_string_into(const std::string &str,
                                    std::vector<uint8_t> &target) {
  pack_into<T>((T)str.size(), target);
  target.insert(target.end(), str.begin(), str.end());
}

//! \brief Unpack an encoded string (with prefixed len) from an index in a given
//! data source
static inline std::optional<std::string>
unpack_string_at(const std::vector<uint8_t> &from, const std::size_t idx) {
  if (from.size() <= (idx + sizeof(std::size_t))) {
    return {};
  }

  std::size_t len = *(std::size_t *)(from.data() + idx);

  if (from.size() < (idx + sizeof(std::size_t) + len)) {
    return {};
  }

  std::string result(from.begin() + idx + sizeof(std::size_t),
                     from.begin() + idx + sizeof(std::size_t) + len);

  return {result};
}

//! \brief Convert a real to an encoded 8-byte chunk
static inline uint64_t real_to_uint64_t(const double &value) {
  return *reinterpret_cast<const uint64_t *>(&value);
}

//! \brief Convert an encoded 8-byte chunk to real
static inline double real_from_uint64_t(const uint64_t &value) {
  return *reinterpret_cast<const double *>(&value);
}

} // namespace pkg::bytes