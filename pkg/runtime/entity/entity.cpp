#include "runtime/entity/entity.hpp"

namespace runtime {

entity_c::entity_c() : id_("") {}

std::string entity_c::get_id() const { return id_; }

} // namespace runtime

