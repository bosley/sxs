#pragma once

#include <string>

namespace runtime {

class entity_c {
public:
  entity_c(const entity_c &) = delete;
  entity_c(entity_c &&) = delete;
  entity_c &operator=(const entity_c &) = delete;
  entity_c &operator=(entity_c &&) = delete;

  entity_c();
  ~entity_c() = default;

  std::string get_id() const;

private:
  std::string id_;
};

} // namespace runtime
