#include "runtime/session/session.hpp"

namespace runtime {

session_c::session_c() : id_(""), active_(false) {}

std::string session_c::get_id() const { return id_; }

bool session_c::is_active() const { return active_; }

} // namespace runtime

