#include <cstring>
#include <kernel_api.hpp>
#include <string>
#include <vector>

static const struct pkg::kernel::api_table_s *g_api = nullptr;

static bool is_list_type(slp::slp_type_e type) {
  return type == slp::slp_type_e::PAREN_LIST ||
         type == slp::slp_type_e::BRACKET_LIST ||
         type == slp::slp_type_e::BRACE_LIST ||
         type == slp::slp_type_e::DQ_LIST;
}

static slp::slp_object_c upcast_to_list(pkg::kernel::context_t ctx,
                                        const slp::slp_object_c &obj) {
  if (is_list_type(obj.type())) {
    return slp::slp_object_c::from_data(obj.get_data(), obj.get_symbols(),
                                        obj.get_root_offset());
  }

  std::vector<slp::slp_object_c> single;
  auto evaled = g_api->eval(ctx, const_cast<slp::slp_object_c &>(obj));
  single.push_back(std::move(evaled));
  return slp::slp_object_c::create_paren_list(single.data(), 1);
}

static slp::slp_object_c
create_list_of_type(slp::slp_type_e type,
                    const std::vector<slp::slp_object_c> &items) {
  switch (type) {
  case slp::slp_type_e::PAREN_LIST:
    return slp::slp_object_c::create_paren_list(items.data(), items.size());
  case slp::slp_type_e::BRACKET_LIST:
    return slp::slp_object_c::create_bracket_list(items.data(), items.size());
  case slp::slp_type_e::BRACE_LIST:
    return slp::slp_object_c::create_brace_list(items.data(), items.size());
  case slp::slp_type_e::DQ_LIST: {
    std::string str;
    for (const auto &item : items) {
      if (item.type() == slp::slp_type_e::INTEGER) {
        str += static_cast<char>(item.as_int());
      } else if (item.type() == slp::slp_type_e::SYMBOL) {
        str += item.as_symbol();
      } else if (item.type() == slp::slp_type_e::DQ_LIST) {
        str += item.as_string().to_string();
      }
    }
    return slp::slp_object_c::create_string(str.c_str());
  }
  default:
    return slp::slp_object_c::create_paren_list(items.data(), items.size());
  }
}

static bool objects_equal(const slp::slp_object_c &a,
                          const slp::slp_object_c &b) {
  if (a.type() != b.type()) {
    return false;
  }

  switch (a.type()) {
  case slp::slp_type_e::INTEGER:
    return a.as_int() == b.as_int();
  case slp::slp_type_e::REAL:
    return a.as_real() == b.as_real();
  case slp::slp_type_e::SYMBOL:
    return std::strcmp(a.as_symbol(), b.as_symbol()) == 0;
  case slp::slp_type_e::DQ_LIST:
    return a.as_string().to_string() == b.as_string().to_string();
  case slp::slp_type_e::NONE:
    return true;
  default:
    return false;
  }
}

static slp::slp_object_c forge_resize(pkg::kernel::context_t ctx,
                                      const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 4) {
    return slp::slp_object_c::create_none();
  }

  auto target = g_api->eval(ctx, list.at(1));
  auto new_size_obj = g_api->eval(ctx, list.at(2));
  auto default_val = g_api->eval(ctx, list.at(3));

  if (new_size_obj.type() != slp::slp_type_e::INTEGER) {
    return slp::slp_object_c::create_none();
  }

  long long new_size = new_size_obj.as_int();
  if (new_size < 0) {
    new_size = 0;
  }

  auto upcast = upcast_to_list(ctx, target);
  auto orig_list = upcast.as_list();
  slp::slp_type_e orig_type = target.type();
  if (!is_list_type(orig_type)) {
    orig_type = slp::slp_type_e::PAREN_LIST;
  }

  std::vector<slp::slp_object_c> items;
  for (size_t i = 0; i < static_cast<size_t>(new_size); i++) {
    if (i < orig_list.size()) {
      items.push_back(slp::slp_object_c::from_data(
          orig_list.at(i).get_data(), orig_list.at(i).get_symbols(),
          orig_list.at(i).get_root_offset()));
    } else {
      items.push_back(std::move(g_api->eval(ctx, default_val)));
    }
  }

  return create_list_of_type(orig_type, items);
}

static slp::slp_object_c forge_pf(pkg::kernel::context_t ctx,
                                  const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 3) {
    return slp::slp_object_c::create_none();
  }

  auto target = g_api->eval(ctx, list.at(1));
  auto obj = g_api->eval(ctx, list.at(2));

  auto upcast = upcast_to_list(ctx, target);
  auto orig_list = upcast.as_list();
  slp::slp_type_e orig_type = target.type();
  if (!is_list_type(orig_type)) {
    orig_type = slp::slp_type_e::PAREN_LIST;
  }

  std::vector<slp::slp_object_c> items;
  items.push_back(std::move(obj));
  for (size_t i = 0; i < orig_list.size(); i++) {
    items.push_back(slp::slp_object_c::from_data(
        orig_list.at(i).get_data(), orig_list.at(i).get_symbols(),
        orig_list.at(i).get_root_offset()));
  }

  return create_list_of_type(orig_type, items);
}

static slp::slp_object_c forge_pb(pkg::kernel::context_t ctx,
                                  const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 3) {
    return slp::slp_object_c::create_none();
  }

  auto target = g_api->eval(ctx, list.at(1));
  auto obj = g_api->eval(ctx, list.at(2));

  auto upcast = upcast_to_list(ctx, target);
  auto orig_list = upcast.as_list();
  slp::slp_type_e orig_type = target.type();
  if (!is_list_type(orig_type)) {
    orig_type = slp::slp_type_e::PAREN_LIST;
  }

  std::vector<slp::slp_object_c> items;
  for (size_t i = 0; i < orig_list.size(); i++) {
    items.push_back(slp::slp_object_c::from_data(
        orig_list.at(i).get_data(), orig_list.at(i).get_symbols(),
        orig_list.at(i).get_root_offset()));
  }
  items.push_back(std::move(obj));

  return create_list_of_type(orig_type, items);
}

static slp::slp_object_c forge_rf(pkg::kernel::context_t ctx,
                                  const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 2) {
    return slp::slp_object_c::create_none();
  }

  auto target = g_api->eval(ctx, list.at(1));
  auto upcast = upcast_to_list(ctx, target);
  auto orig_list = upcast.as_list();
  slp::slp_type_e orig_type = target.type();
  if (!is_list_type(orig_type)) {
    orig_type = slp::slp_type_e::PAREN_LIST;
  }

  if (orig_list.empty()) {
    std::vector<slp::slp_object_c> empty;
    return create_list_of_type(orig_type, empty);
  }

  std::vector<slp::slp_object_c> items;
  for (size_t i = 1; i < orig_list.size(); i++) {
    items.push_back(slp::slp_object_c::from_data(
        orig_list.at(i).get_data(), orig_list.at(i).get_symbols(),
        orig_list.at(i).get_root_offset()));
  }

  return create_list_of_type(orig_type, items);
}

static slp::slp_object_c forge_rb(pkg::kernel::context_t ctx,
                                  const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 2) {
    return slp::slp_object_c::create_none();
  }

  auto target = g_api->eval(ctx, list.at(1));
  auto upcast = upcast_to_list(ctx, target);
  auto orig_list = upcast.as_list();
  slp::slp_type_e orig_type = target.type();
  if (!is_list_type(orig_type)) {
    orig_type = slp::slp_type_e::PAREN_LIST;
  }

  if (orig_list.empty()) {
    std::vector<slp::slp_object_c> empty;
    return create_list_of_type(orig_type, empty);
  }

  std::vector<slp::slp_object_c> items;
  for (size_t i = 0; i < orig_list.size() - 1; i++) {
    items.push_back(slp::slp_object_c::from_data(
        orig_list.at(i).get_data(), orig_list.at(i).get_symbols(),
        orig_list.at(i).get_root_offset()));
  }

  return create_list_of_type(orig_type, items);
}

static slp::slp_object_c forge_lsh(pkg::kernel::context_t ctx,
                                   const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 3) {
    return slp::slp_object_c::create_none();
  }

  auto target = g_api->eval(ctx, list.at(1));
  auto count_obj = g_api->eval(ctx, list.at(2));

  if (count_obj.type() != slp::slp_type_e::INTEGER) {
    return slp::slp_object_c::create_none();
  }

  long long count = count_obj.as_int();
  if (count <= 0) {
    return target;
  }

  auto upcast = upcast_to_list(ctx, target);
  auto orig_list = upcast.as_list();
  slp::slp_type_e orig_type = target.type();
  if (!is_list_type(orig_type)) {
    orig_type = slp::slp_type_e::PAREN_LIST;
  }

  if (orig_list.empty()) {
    std::vector<slp::slp_object_c> empty;
    return create_list_of_type(orig_type, empty);
  }

  size_t actual_shift = static_cast<size_t>(count) % orig_list.size();
  if (actual_shift == 0) {
    std::vector<slp::slp_object_c> items;
    for (size_t i = 0; i < orig_list.size(); i++) {
      items.push_back(slp::slp_object_c::from_data(
          orig_list.at(i).get_data(), orig_list.at(i).get_symbols(),
          orig_list.at(i).get_root_offset()));
    }
    return create_list_of_type(orig_type, items);
  }

  std::vector<slp::slp_object_c> items;
  for (size_t i = actual_shift; i < orig_list.size(); i++) {
    items.push_back(slp::slp_object_c::from_data(
        orig_list.at(i).get_data(), orig_list.at(i).get_symbols(),
        orig_list.at(i).get_root_offset()));
  }

  return create_list_of_type(orig_type, items);
}

static slp::slp_object_c forge_rsh(pkg::kernel::context_t ctx,
                                   const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 3) {
    return slp::slp_object_c::create_none();
  }

  auto target = g_api->eval(ctx, list.at(1));
  auto count_obj = g_api->eval(ctx, list.at(2));

  if (count_obj.type() != slp::slp_type_e::INTEGER) {
    return slp::slp_object_c::create_none();
  }

  long long count = count_obj.as_int();
  if (count <= 0) {
    return target;
  }

  auto upcast = upcast_to_list(ctx, target);
  auto orig_list = upcast.as_list();
  slp::slp_type_e orig_type = target.type();
  if (!is_list_type(orig_type)) {
    orig_type = slp::slp_type_e::PAREN_LIST;
  }

  if (orig_list.empty()) {
    std::vector<slp::slp_object_c> empty;
    return create_list_of_type(orig_type, empty);
  }

  size_t actual_shift = static_cast<size_t>(count) % orig_list.size();
  if (actual_shift == 0) {
    std::vector<slp::slp_object_c> items;
    for (size_t i = 0; i < orig_list.size(); i++) {
      items.push_back(slp::slp_object_c::from_data(
          orig_list.at(i).get_data(), orig_list.at(i).get_symbols(),
          orig_list.at(i).get_root_offset()));
    }
    return create_list_of_type(orig_type, items);
  }

  size_t new_size = orig_list.size() - actual_shift;
  std::vector<slp::slp_object_c> items;
  for (size_t i = 0; i < new_size; i++) {
    items.push_back(slp::slp_object_c::from_data(
        orig_list.at(i).get_data(), orig_list.at(i).get_symbols(),
        orig_list.at(i).get_root_offset()));
  }

  return create_list_of_type(orig_type, items);
}

static slp::slp_object_c forge_rotr(pkg::kernel::context_t ctx,
                                    const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 3) {
    return slp::slp_object_c::create_none();
  }

  auto target = g_api->eval(ctx, list.at(1));
  auto count_obj = g_api->eval(ctx, list.at(2));

  if (count_obj.type() != slp::slp_type_e::INTEGER) {
    return slp::slp_object_c::create_none();
  }

  long long count = count_obj.as_int();
  if (count <= 0) {
    return target;
  }

  auto upcast = upcast_to_list(ctx, target);
  auto orig_list = upcast.as_list();
  slp::slp_type_e orig_type = target.type();
  if (!is_list_type(orig_type)) {
    orig_type = slp::slp_type_e::PAREN_LIST;
  }

  if (orig_list.empty() || orig_list.size() == 1) {
    std::vector<slp::slp_object_c> items;
    for (size_t i = 0; i < orig_list.size(); i++) {
      items.push_back(slp::slp_object_c::from_data(
          orig_list.at(i).get_data(), orig_list.at(i).get_symbols(),
          orig_list.at(i).get_root_offset()));
    }
    return create_list_of_type(orig_type, items);
  }

  size_t actual_rotations = static_cast<size_t>(count) % orig_list.size();
  if (actual_rotations == 0) {
    std::vector<slp::slp_object_c> items;
    for (size_t i = 0; i < orig_list.size(); i++) {
      items.push_back(slp::slp_object_c::from_data(
          orig_list.at(i).get_data(), orig_list.at(i).get_symbols(),
          orig_list.at(i).get_root_offset()));
    }
    return create_list_of_type(orig_type, items);
  }

  std::vector<slp::slp_object_c> items;
  size_t start_pos = orig_list.size() - actual_rotations;
  for (size_t i = 0; i < orig_list.size(); i++) {
    size_t idx = (start_pos + i) % orig_list.size();
    items.push_back(slp::slp_object_c::from_data(
        orig_list.at(idx).get_data(), orig_list.at(idx).get_symbols(),
        orig_list.at(idx).get_root_offset()));
  }

  return create_list_of_type(orig_type, items);
}

static slp::slp_object_c forge_rotl(pkg::kernel::context_t ctx,
                                    const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 3) {
    return slp::slp_object_c::create_none();
  }

  auto target = g_api->eval(ctx, list.at(1));
  auto count_obj = g_api->eval(ctx, list.at(2));

  if (count_obj.type() != slp::slp_type_e::INTEGER) {
    return slp::slp_object_c::create_none();
  }

  long long count = count_obj.as_int();
  if (count <= 0) {
    return target;
  }

  auto upcast = upcast_to_list(ctx, target);
  auto orig_list = upcast.as_list();
  slp::slp_type_e orig_type = target.type();
  if (!is_list_type(orig_type)) {
    orig_type = slp::slp_type_e::PAREN_LIST;
  }

  if (orig_list.empty() || orig_list.size() == 1) {
    std::vector<slp::slp_object_c> items;
    for (size_t i = 0; i < orig_list.size(); i++) {
      items.push_back(slp::slp_object_c::from_data(
          orig_list.at(i).get_data(), orig_list.at(i).get_symbols(),
          orig_list.at(i).get_root_offset()));
    }
    return create_list_of_type(orig_type, items);
  }

  size_t actual_rotations = static_cast<size_t>(count) % orig_list.size();
  if (actual_rotations == 0) {
    std::vector<slp::slp_object_c> items;
    for (size_t i = 0; i < orig_list.size(); i++) {
      items.push_back(slp::slp_object_c::from_data(
          orig_list.at(i).get_data(), orig_list.at(i).get_symbols(),
          orig_list.at(i).get_root_offset()));
    }
    return create_list_of_type(orig_type, items);
  }

  std::vector<slp::slp_object_c> items;
  for (size_t i = 0; i < orig_list.size(); i++) {
    size_t idx = (actual_rotations + i) % orig_list.size();
    items.push_back(slp::slp_object_c::from_data(
        orig_list.at(idx).get_data(), orig_list.at(idx).get_symbols(),
        orig_list.at(idx).get_root_offset()));
  }

  return create_list_of_type(orig_type, items);
}

static slp::slp_object_c forge_rev(pkg::kernel::context_t ctx,
                                   const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 2) {
    return slp::slp_object_c::create_none();
  }

  auto target = g_api->eval(ctx, list.at(1));
  auto upcast = upcast_to_list(ctx, target);
  auto orig_list = upcast.as_list();
  slp::slp_type_e orig_type = target.type();
  if (!is_list_type(orig_type)) {
    orig_type = slp::slp_type_e::PAREN_LIST;
  }

  std::vector<slp::slp_object_c> items;
  for (size_t i = orig_list.size(); i > 0; i--) {
    items.push_back(slp::slp_object_c::from_data(
        orig_list.at(i - 1).get_data(), orig_list.at(i - 1).get_symbols(),
        orig_list.at(i - 1).get_root_offset()));
  }

  return create_list_of_type(orig_type, items);
}

static slp::slp_object_c forge_count(pkg::kernel::context_t ctx,
                                     const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 2) {
    return slp::slp_object_c::create_int(0);
  }

  auto target = g_api->eval(ctx, list.at(1));

  if (target.type() == slp::slp_type_e::DQ_LIST) {
    auto str = target.as_string();
    return slp::slp_object_c::create_int(static_cast<long long>(str.size()));
  }

  auto upcast = upcast_to_list(ctx, target);
  auto orig_list = upcast.as_list();

  return slp::slp_object_c::create_int(
      static_cast<long long>(orig_list.size()));
}

static slp::slp_object_c forge_concat(pkg::kernel::context_t ctx,
                                      const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 3) {
    return slp::slp_object_c::create_none();
  }

  auto target = g_api->eval(ctx, list.at(1));
  auto other = g_api->eval(ctx, list.at(2));

  auto upcast1 = upcast_to_list(ctx, target);
  auto upcast2 = upcast_to_list(ctx, other);
  auto list1 = upcast1.as_list();
  auto list2 = upcast2.as_list();

  slp::slp_type_e orig_type = target.type();
  if (!is_list_type(orig_type)) {
    orig_type = slp::slp_type_e::PAREN_LIST;
  }

  std::vector<slp::slp_object_c> items;
  for (size_t i = 0; i < list1.size(); i++) {
    items.push_back(slp::slp_object_c::from_data(
        list1.at(i).get_data(), list1.at(i).get_symbols(),
        list1.at(i).get_root_offset()));
  }
  for (size_t i = 0; i < list2.size(); i++) {
    items.push_back(slp::slp_object_c::from_data(
        list2.at(i).get_data(), list2.at(i).get_symbols(),
        list2.at(i).get_root_offset()));
  }

  return create_list_of_type(orig_type, items);
}

static slp::slp_object_c forge_replace(pkg::kernel::context_t ctx,
                                       const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 4) {
    return slp::slp_object_c::create_none();
  }

  auto target = g_api->eval(ctx, list.at(1));
  auto match = g_api->eval(ctx, list.at(2));
  auto replacement = g_api->eval(ctx, list.at(3));

  auto upcast = upcast_to_list(ctx, target);
  auto orig_list = upcast.as_list();
  slp::slp_type_e orig_type = target.type();
  if (!is_list_type(orig_type)) {
    orig_type = slp::slp_type_e::PAREN_LIST;
  }

  std::vector<slp::slp_object_c> items;
  for (size_t i = 0; i < orig_list.size(); i++) {
    auto item = g_api->eval(ctx, orig_list.at(i));
    if (objects_equal(item, match)) {
      items.push_back(std::move(g_api->eval(ctx, replacement)));
    } else {
      items.push_back(std::move(item));
    }
  }

  return create_list_of_type(orig_type, items);
}

static slp::slp_object_c forge_drop_match(pkg::kernel::context_t ctx,
                                          const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 3) {
    return slp::slp_object_c::create_none();
  }

  auto target = g_api->eval(ctx, list.at(1));
  auto match = g_api->eval(ctx, list.at(2));

  auto upcast = upcast_to_list(ctx, target);
  auto orig_list = upcast.as_list();
  slp::slp_type_e orig_type = target.type();
  if (!is_list_type(orig_type)) {
    orig_type = slp::slp_type_e::PAREN_LIST;
  }

  std::vector<slp::slp_object_c> items;
  for (size_t i = 0; i < orig_list.size(); i++) {
    auto item = g_api->eval(ctx, orig_list.at(i));
    if (!objects_equal(item, match)) {
      items.push_back(std::move(item));
    }
  }

  return create_list_of_type(orig_type, items);
}

static slp::slp_object_c forge_drop_period(pkg::kernel::context_t ctx,
                                           const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 4) {
    return slp::slp_object_c::create_none();
  }

  auto target = g_api->eval(ctx, list.at(1));
  auto start_obj = g_api->eval(ctx, list.at(2));
  auto period_obj = g_api->eval(ctx, list.at(3));

  if (start_obj.type() != slp::slp_type_e::INTEGER ||
      period_obj.type() != slp::slp_type_e::INTEGER) {
    return slp::slp_object_c::create_none();
  }

  long long start = start_obj.as_int();
  long long period = period_obj.as_int();

  if (period <= 0) {
    return target;
  }

  auto upcast = upcast_to_list(ctx, target);
  auto orig_list = upcast.as_list();
  slp::slp_type_e orig_type = target.type();
  if (!is_list_type(orig_type)) {
    orig_type = slp::slp_type_e::PAREN_LIST;
  }

  std::vector<slp::slp_object_c> items;
  for (size_t i = 0; i < orig_list.size(); i++) {
    long long idx = static_cast<long long>(i);
    if (idx < start || (idx - start) % period != 0) {
      items.push_back(slp::slp_object_c::from_data(
          orig_list.at(i).get_data(), orig_list.at(i).get_symbols(),
          orig_list.at(i).get_root_offset()));
    }
  }

  return create_list_of_type(orig_type, items);
}

static slp::slp_object_c forge_to_bits(pkg::kernel::context_t ctx,
                                       const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 2) {
    std::vector<slp::slp_object_c> empty;
    return slp::slp_object_c::create_brace_list(empty.data(), 0);
  }

  auto value_obj = g_api->eval(ctx, list.at(1));
  if (value_obj.type() != slp::slp_type_e::INTEGER) {
    std::vector<slp::slp_object_c> empty;
    return slp::slp_object_c::create_brace_list(empty.data(), 0);
  }

  long long value = value_obj.as_int();
  std::uint64_t bits = static_cast<std::uint64_t>(value);

  std::vector<slp::slp_object_c> bit_objects;
  bit_objects.reserve(64);

  for (int i = 63; i >= 0; i--) {
    std::uint64_t bit = (bits >> i) & 1ULL;
    bit_objects.push_back(
        slp::slp_object_c::create_int(static_cast<long long>(bit)));
  }

  return slp::slp_object_c::create_brace_list(bit_objects.data(),
                                              bit_objects.size());
}

static slp::slp_object_c forge_from_bits(pkg::kernel::context_t ctx,
                                         const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 2) {
    return slp::slp_object_c::create_int(0);
  }

  auto bits_obj = g_api->eval(ctx, list.at(1));

  if (!is_list_type(bits_obj.type())) {
    return slp::slp_object_c::create_int(0);
  }

  auto bits_list = bits_obj.as_list();
  std::uint64_t result = 0;

  for (size_t i = 0; i < bits_list.size() && i < 64; i++) {
    auto bit_obj = g_api->eval(ctx, bits_list.at(i));
    if (bit_obj.type() == slp::slp_type_e::INTEGER) {
      long long bit_val = bit_obj.as_int();
      if (bit_val != 0) {
        size_t shift = bits_list.size() - 1 - i;
        if (shift < 64) {
          result |= (1ULL << shift);
        }
      }
    }
  }

  return slp::slp_object_c::create_int(static_cast<long long>(result));
}

static slp::slp_object_c forge_to_bits_r(pkg::kernel::context_t ctx,
                                         const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 2) {
    std::vector<slp::slp_object_c> empty;
    return slp::slp_object_c::create_brace_list(empty.data(), 0);
  }

  auto value_obj = g_api->eval(ctx, list.at(1));
  if (value_obj.type() != slp::slp_type_e::REAL) {
    std::vector<slp::slp_object_c> empty;
    return slp::slp_object_c::create_brace_list(empty.data(), 0);
  }

  double value = value_obj.as_real();
  std::uint64_t bits;
  std::memcpy(&bits, &value, sizeof(double));

  std::vector<slp::slp_object_c> bit_objects;
  bit_objects.reserve(64);

  for (int i = 63; i >= 0; i--) {
    std::uint64_t bit = (bits >> i) & 1ULL;
    bit_objects.push_back(
        slp::slp_object_c::create_int(static_cast<long long>(bit)));
  }

  return slp::slp_object_c::create_brace_list(bit_objects.data(),
                                              bit_objects.size());
}

static slp::slp_object_c forge_from_bits_r(pkg::kernel::context_t ctx,
                                           const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 2) {
    return slp::slp_object_c::create_real(0.0);
  }

  auto bits_obj = g_api->eval(ctx, list.at(1));

  if (!is_list_type(bits_obj.type())) {
    return slp::slp_object_c::create_real(0.0);
  }

  auto bits_list = bits_obj.as_list();
  std::uint64_t result = 0;

  for (size_t i = 0; i < bits_list.size() && i < 64; i++) {
    auto bit_obj = g_api->eval(ctx, bits_list.at(i));
    if (bit_obj.type() == slp::slp_type_e::INTEGER) {
      long long bit_val = bit_obj.as_int();
      if (bit_val != 0) {
        size_t shift = bits_list.size() - 1 - i;
        if (shift < 64) {
          result |= (1ULL << shift);
        }
      }
    }
  }

  double real_value;
  std::memcpy(&real_value, &result, sizeof(double));
  return slp::slp_object_c::create_real(real_value);
}

extern "C" void kernel_init(pkg::kernel::registry_t registry,
                            const struct pkg::kernel::api_table_s *api) {
  g_api = api;
  api->register_function(registry, "resize", forge_resize,
                         slp::slp_type_e::NONE, 0);
  api->register_function(registry, "pf", forge_pf, slp::slp_type_e::NONE, 0);
  api->register_function(registry, "pb", forge_pb, slp::slp_type_e::NONE, 0);
  api->register_function(registry, "rf", forge_rf, slp::slp_type_e::NONE, 0);
  api->register_function(registry, "rb", forge_rb, slp::slp_type_e::NONE, 0);
  api->register_function(registry, "lsh", forge_lsh, slp::slp_type_e::NONE, 0);
  api->register_function(registry, "rsh", forge_rsh, slp::slp_type_e::NONE, 0);
  api->register_function(registry, "rotr", forge_rotr, slp::slp_type_e::NONE,
                         0);
  api->register_function(registry, "rotl", forge_rotl, slp::slp_type_e::NONE,
                         0);
  api->register_function(registry, "rev", forge_rev, slp::slp_type_e::NONE, 0);
  api->register_function(registry, "count", forge_count,
                         slp::slp_type_e::INTEGER, 0);
  api->register_function(registry, "concat", forge_concat,
                         slp::slp_type_e::NONE, 0);
  api->register_function(registry, "replace", forge_replace,
                         slp::slp_type_e::NONE, 0);
  api->register_function(registry, "drop_match", forge_drop_match,
                         slp::slp_type_e::NONE, 0);
  api->register_function(registry, "drop_period", forge_drop_period,
                         slp::slp_type_e::NONE, 0);
  api->register_function(registry, "to_bits", forge_to_bits,
                         slp::slp_type_e::BRACE_LIST, 0);
  api->register_function(registry, "from_bits", forge_from_bits,
                         slp::slp_type_e::INTEGER, 0);
  api->register_function(registry, "to_bits_r", forge_to_bits_r,
                         slp::slp_type_e::BRACE_LIST, 0);
  api->register_function(registry, "from_bits_r", forge_from_bits_r,
                         slp::slp_type_e::REAL, 0);
}
