#include <cmath>
#include <kernel_api.hpp>
#include <vector>

static const struct pkg::kernel::api_table_s *g_api = nullptr;

static slp::slp_object_c forms_make_pair(pkg::kernel::context_t ctx,
                                         const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 3) {
    return slp::slp_object_c::create_int(0);
  }

  auto a = g_api->eval(ctx, list.at(1));
  auto b = g_api->eval(ctx, list.at(2));

  slp::slp_object_c elements[2] = {std::move(a), std::move(b)};
  return slp::slp_object_c::create_brace_list(elements, 2);
}

static slp::slp_object_c forms_sum_pair(pkg::kernel::context_t ctx,
                                        const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 2) {
    return slp::slp_object_c::create_int(0);
  }

  auto pair_obj = g_api->eval(ctx, list.at(1));
  if (pair_obj.type() != slp::slp_type_e::BRACE_LIST) {
    return slp::slp_object_c::create_int(0);
  }

  auto pair_list = pair_obj.as_list();
  if (pair_list.size() != 2) {
    return slp::slp_object_c::create_int(0);
  }

  auto first = pair_list.at(0).as_int();
  auto second = pair_list.at(1).as_int();

  return slp::slp_object_c::create_int(first + second);
}

static slp::slp_object_c forms_make_result(pkg::kernel::context_t ctx,
                                           const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 4) {
    return slp::slp_object_c::create_int(0);
  }

  auto msg = g_api->eval(ctx, list.at(1));
  auto code = g_api->eval(ctx, list.at(2));
  auto err = g_api->eval(ctx, list.at(3));

  slp::slp_object_c elements[3] = {std::move(msg), std::move(code),
                                   std::move(err)};
  return slp::slp_object_c::create_brace_list(elements, 3);
}

static slp::slp_object_c forms_process_batch(pkg::kernel::context_t ctx,
                                             const slp::slp_object_c &args) {
  auto list = args.as_list();

  int total_pairs = 0;
  int sum = 0;

  for (size_t i = 1; i < list.size(); i++) {
    auto pair_obj = g_api->eval(ctx, list.at(i));
    if (pair_obj.type() == slp::slp_type_e::BRACE_LIST) {
      auto pair_list = pair_obj.as_list();
      if (pair_list.size() == 2) {
        total_pairs++;
        sum += pair_list.at(0).as_int();
        sum += pair_list.at(1).as_int();
      }
    }
  }

  auto msg = slp::slp_object_c::create_string("batch processed");
  auto code = slp::slp_object_c::create_int(sum);
  auto err = slp::slp_object_c::create_none();

  slp::slp_object_c elements[3] = {std::move(msg), std::move(code),
                                   std::move(err)};
  return slp::slp_object_c::create_brace_list(elements, 3);
}

static slp::slp_object_c forms_make_point(pkg::kernel::context_t ctx,
                                          const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 3) {
    return slp::slp_object_c::create_real(0.0);
  }

  auto x = g_api->eval(ctx, list.at(1));
  auto y = g_api->eval(ctx, list.at(2));

  slp::slp_object_c elements[2] = {std::move(x), std::move(y)};
  return slp::slp_object_c::create_brace_list(elements, 2);
}

static slp::slp_object_c forms_distance(pkg::kernel::context_t ctx,
                                        const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 3) {
    return slp::slp_object_c::create_real(0.0);
  }

  auto p1_obj = g_api->eval(ctx, list.at(1));
  auto p2_obj = g_api->eval(ctx, list.at(2));

  if (p1_obj.type() != slp::slp_type_e::BRACE_LIST ||
      p2_obj.type() != slp::slp_type_e::BRACE_LIST) {
    return slp::slp_object_c::create_real(0.0);
  }

  auto p1_list = p1_obj.as_list();
  auto p2_list = p2_obj.as_list();

  if (p1_list.size() != 2 || p2_list.size() != 2) {
    return slp::slp_object_c::create_real(0.0);
  }

  double x1 = p1_list.at(0).as_real();
  double y1 = p1_list.at(1).as_real();
  double x2 = p2_list.at(0).as_real();
  double y2 = p2_list.at(1).as_real();

  double dx = x2 - x1;
  double dy = y2 - y1;
  double dist = std::sqrt(dx * dx + dy * dy);

  return slp::slp_object_c::create_real(dist);
}

extern "C" void kernel_init(pkg::kernel::registry_t registry,
                            const struct pkg::kernel::api_table_s *api) {
  g_api = api;
  api->register_function(registry, "make_pair", forms_make_pair,
                         slp::slp_type_e::BRACE_LIST, 0);
  api->register_function(registry, "sum_pair", forms_sum_pair,
                         slp::slp_type_e::INTEGER, 0);
  api->register_function(registry, "make_result", forms_make_result,
                         slp::slp_type_e::BRACE_LIST, 0);
  api->register_function(registry, "process_batch", forms_process_batch,
                         slp::slp_type_e::BRACE_LIST, 1);
  api->register_function(registry, "make_point", forms_make_point,
                         slp::slp_type_e::BRACE_LIST, 0);
  api->register_function(registry, "distance", forms_distance,
                         slp::slp_type_e::REAL, 0);
}

extern "C" void kernel_shutdown(const struct pkg::kernel::api_table_s *api) {}
