#pragma once

#include <string>

namespace runtime {

class session_c {
public:
  session_c(const session_c &) = delete;
  session_c(session_c &&) = delete;
  session_c &operator=(const session_c &) = delete;
  session_c &operator=(session_c &&) = delete;

  session_c();
  ~session_c() = default;

  std::string get_id() const;
  bool is_active() const;

private:
  std::string id_;
  bool active_;
};

} // namespace runtime
