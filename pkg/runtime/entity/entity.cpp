#include "runtime/entity/entity.hpp"
#include <slp/slp.hpp>

namespace runtime {

entity_c::entity_c() {
  field_values_.resize(2);
  field_values_[0] = "[]";
  field_values_[1] = "[]";
}

std::string entity_c::get_type_id() const { return "entity"; }

std::string entity_c::get_schema() const {
  return R"([(permissions "[]") (topic_permissions "[]")])";
}

size_t entity_c::field_count() const { return 2; }

bool entity_c::get_field(size_t index, std::string &value) {
  if (index >= field_values_.size()) {
    return false;
  }
  value = field_values_[index];
  return true;
}

bool entity_c::set_field(size_t index, const std::string &value) {
  if (index >= field_values_.size()) {
    return false;
  }
  field_values_[index] = value;
  return true;
}

bool entity_c::load() {
  if (!manager_) {
    return false;
  }

  std::string key = manager_->make_data_key(get_type_id(), instance_id_, 0);
  if (!manager_->get_store().get(key, field_values_[0])) {
    return false;
  }

  deserialize_permissions(field_values_[0]);

  key = manager_->make_data_key(get_type_id(), instance_id_, 1);
  if (manager_->get_store().get(key, field_values_[1])) {
    deserialize_topic_permissions(field_values_[1]);
  }

  return true;
}

std::string entity_c::get_id() const { return instance_id_; }

bool entity_c::is_permitted(const std::string &scope,
                            const char *permission) const {
  auto it = permissions_.find(scope);
  if (it == permissions_.end()) {
    return false;
  }

  const std::string &granted = it->second;
  std::string required(permission);

  if (granted == required) {
    return true;
  }

  if (granted == "RW") {
    return required == "R" || required == "W" || required == "RW";
  }

  return false;
}

void entity_c::grant_permission(const std::string &scope,
                                const char *permission) {
  permissions_[scope] = std::string(permission);
  field_values_[0] = serialize_permissions();
}

void entity_c::revoke_permission(const std::string &scope) {
  permissions_.erase(scope);
  field_values_[0] = serialize_permissions();
}

std::map<std::string, std::string> entity_c::get_permissions() const {
  return permissions_;
}

void entity_c::set_permissions(
    const std::map<std::string, std::string> &perms) {
  permissions_ = perms;
  field_values_[0] = serialize_permissions();
}

std::string entity_c::serialize_permissions() const {
  if (permissions_.empty()) {
    return "[]";
  }

  std::string result = "[";
  bool first = true;
  for (const auto &pair : permissions_) {
    if (!first) {
      result += " ";
    }
    first = false;
    result += "(" + pair.first + " \"" + pair.second + "\")";
  }
  result += "]";

  return result;
}

void entity_c::deserialize_permissions(const std::string &data) {
  permissions_.clear();

  if (data.empty() || data == "[]") {
    return;
  }

  auto parse_result = slp::parse(data);
  if (parse_result.is_error()) {
    return;
  }

  const auto &root = parse_result.object();
  if (root.type() != slp::slp_type_e::BRACKET_LIST) {
    return;
  }

  auto list = root.as_list();
  for (size_t i = 0; i < list.size(); ++i) {
    auto pair = list.at(i);
    if (pair.type() != slp::slp_type_e::PAREN_LIST) {
      continue;
    }

    auto pair_list = pair.as_list();
    if (pair_list.size() != 2) {
      continue;
    }

    auto scope_obj = pair_list.at(0);
    auto perm_obj = pair_list.at(1);

    if (scope_obj.type() != slp::slp_type_e::SYMBOL) {
      continue;
    }

    if (perm_obj.type() != slp::slp_type_e::DQ_LIST) {
      continue;
    }

    std::string scope = scope_obj.as_symbol();
    std::string perm = perm_obj.as_string().to_string();

    permissions_[scope] = perm;
  }
}

bool entity_c::is_permitted_topic(std::uint16_t topic_id,
                                  const char *permission) const {
  auto it = topic_permissions_.find(topic_id);
  if (it == topic_permissions_.end()) {
    return false;
  }

  const std::string &granted = it->second;
  std::string required(permission);

  if (granted == required) {
    return true;
  }

  if (granted == "PS") {
    return required == "P" || required == "S" || required == "PS";
  }

  return false;
}

void entity_c::grant_topic_permission(std::uint16_t topic_id,
                                      const char *permission) {
  topic_permissions_[topic_id] = std::string(permission);
  field_values_[1] = serialize_topic_permissions();
}

void entity_c::revoke_topic_permission(std::uint16_t topic_id) {
  topic_permissions_.erase(topic_id);
  field_values_[1] = serialize_topic_permissions();
}

std::map<std::uint16_t, std::string> entity_c::get_topic_permissions() const {
  return topic_permissions_;
}

std::string entity_c::serialize_topic_permissions() const {
  if (topic_permissions_.empty()) {
    return "[]";
  }

  std::string result = "[";
  bool first = true;
  for (const auto &pair : topic_permissions_) {
    if (!first) {
      result += " ";
    }
    first = false;
    result += "(" + std::to_string(pair.first) + " \"" + pair.second + "\")";
  }
  result += "]";

  return result;
}

void entity_c::deserialize_topic_permissions(const std::string &data) {
  topic_permissions_.clear();

  if (data.empty() || data == "[]") {
    return;
  }

  auto parse_result = slp::parse(data);
  if (parse_result.is_error()) {
    return;
  }

  const auto &root = parse_result.object();
  if (root.type() != slp::slp_type_e::BRACKET_LIST) {
    return;
  }

  auto list = root.as_list();
  for (size_t i = 0; i < list.size(); ++i) {
    auto pair = list.at(i);
    if (pair.type() != slp::slp_type_e::PAREN_LIST) {
      continue;
    }

    auto pair_list = pair.as_list();
    if (pair_list.size() != 2) {
      continue;
    }

    auto topic_obj = pair_list.at(0);
    auto perm_obj = pair_list.at(1);

    if (topic_obj.type() != slp::slp_type_e::INTEGER) {
      continue;
    }

    if (perm_obj.type() != slp::slp_type_e::DQ_LIST) {
      continue;
    }

    std::uint16_t topic_id = static_cast<std::uint16_t>(topic_obj.as_int());
    std::string perm = perm_obj.as_string().to_string();

    topic_permissions_[topic_id] = perm;
  }
}

} // namespace runtime
