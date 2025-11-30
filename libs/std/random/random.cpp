#include "random.hpp"
#include "../../pkg/random/generator.hpp"
#include "../../pkg/random/string.hpp"

static const struct pkg::kernel::api_table_s *g_api = nullptr;

static slp::slp_object_c random_int_range(pkg::kernel::context_t ctx,
                                          const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 3) {
    return slp::slp_object_c::create_int(-1);
  }

  auto evaled_min = g_api->eval(ctx, list.at(1));
  auto evaled_max = g_api->eval(ctx, list.at(2));

  if (evaled_min.type() != slp::slp_type_e::INTEGER ||
      evaled_max.type() != slp::slp_type_e::INTEGER) {
    return slp::slp_object_c::create_int(-1);
  }

  long long min_val = evaled_min.as_int();
  long long max_val = evaled_max.as_int();

  pkg::random::generate_random_c<long long> gen;
  long long result = gen.get_range(min_val, max_val);

  return slp::slp_object_c::create_int(result);
}

static slp::slp_object_c random_real_range(pkg::kernel::context_t ctx,
                                           const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 3) {
    return slp::slp_object_c::create_int(-1);
  }

  auto evaled_min = g_api->eval(ctx, list.at(1));
  auto evaled_max = g_api->eval(ctx, list.at(2));

  if (evaled_min.type() != slp::slp_type_e::REAL ||
      evaled_max.type() != slp::slp_type_e::REAL) {
    return slp::slp_object_c::create_int(-1);
  }

  double min_val = evaled_min.as_real();
  double max_val = evaled_max.as_real();

  pkg::random::generate_random_c<double> gen;
  double result = gen.get_floating_point_range(min_val, max_val);

  return slp::slp_object_c::create_real(result);
}

static slp::slp_object_c random_string(pkg::kernel::context_t ctx,
                                       const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 2) {
    return slp::slp_object_c::create_int(-1);
  }

  auto evaled_length = g_api->eval(ctx, list.at(1));

  if (evaled_length.type() != slp::slp_type_e::INTEGER) {
    return slp::slp_object_c::create_int(-1);
  }

  long long length = evaled_length.as_int();
  if (length < 0) {
    return slp::slp_object_c::create_int(-1);
  }

  pkg::random::random_string_c gen;
  std::string result = gen.generate_string(static_cast<size_t>(length));

  return slp::slp_object_c::create_string(result);
}

static slp::slp_object_c random_string_alpha(pkg::kernel::context_t ctx,
                                             const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 2) {
    return slp::slp_object_c::create_int(-1);
  }

  auto evaled_length = g_api->eval(ctx, list.at(1));

  if (evaled_length.type() != slp::slp_type_e::INTEGER) {
    return slp::slp_object_c::create_int(-1);
  }

  long long length = evaled_length.as_int();
  if (length < 0) {
    return slp::slp_object_c::create_int(-1);
  }

  pkg::random::random_string_c gen(pkg::random::random_string_c::ALPHA_NUM);
  std::string result = gen.generate_string(static_cast<size_t>(length));

  return slp::slp_object_c::create_string(result);
}

extern "C" void kernel_init(pkg::kernel::registry_t registry,
                            const struct pkg::kernel::api_table_s *api) {
  g_api = api;
  api->register_function(registry, "int_range", random_int_range,
                         slp::slp_type_e::INTEGER, 0);
  api->register_function(registry, "real_range", random_real_range,
                         slp::slp_type_e::REAL, 0);
  api->register_function(registry, "string", random_string,
                         slp::slp_type_e::DQ_LIST, 0);
  api->register_function(registry, "string_alpha", random_string_alpha,
                         slp::slp_type_e::DQ_LIST, 0);
}
