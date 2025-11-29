#include "random.hpp"
#include "../../pkg/random/generator.hpp"
#include "../../pkg/random/string.hpp"

static const struct sxs_api_table_t *g_api = nullptr;

static sxs_object_t random_int_range(sxs_context_t ctx, sxs_object_t args) {
  void *list = g_api->as_list(args);
  size_t size = g_api->list_size(list);

  if (size < 3) {
    return g_api->create_int(-1);
  }

  sxs_object_t min_obj = g_api->list_at(list, 1);
  sxs_object_t max_obj = g_api->list_at(list, 2);

  sxs_object_t evaled_min = g_api->eval(ctx, min_obj);
  sxs_object_t evaled_max = g_api->eval(ctx, max_obj);

  if (g_api->get_type(evaled_min) != SXS_TYPE_INT ||
      g_api->get_type(evaled_max) != SXS_TYPE_INT) {
    return g_api->create_int(-1);
  }

  long long min_val = g_api->as_int(evaled_min);
  long long max_val = g_api->as_int(evaled_max);

  pkg::random::generate_random_c<long long> gen;
  long long result = gen.get_range(min_val, max_val);

  return g_api->create_int(result);
}

static sxs_object_t random_real_range(sxs_context_t ctx, sxs_object_t args) {
  void *list = g_api->as_list(args);
  size_t size = g_api->list_size(list);

  if (size < 3) {
    return g_api->create_int(-1);
  }

  sxs_object_t min_obj = g_api->list_at(list, 1);
  sxs_object_t max_obj = g_api->list_at(list, 2);

  sxs_object_t evaled_min = g_api->eval(ctx, min_obj);
  sxs_object_t evaled_max = g_api->eval(ctx, max_obj);

  if (g_api->get_type(evaled_min) != SXS_TYPE_REAL ||
      g_api->get_type(evaled_max) != SXS_TYPE_REAL) {
    return g_api->create_int(-1);
  }

  double min_val = g_api->as_real(evaled_min);
  double max_val = g_api->as_real(evaled_max);

  pkg::random::generate_random_c<double> gen;
  double result = gen.get_floating_point_range(min_val, max_val);

  return g_api->create_real(result);
}

static sxs_object_t random_string(sxs_context_t ctx, sxs_object_t args) {
  void *list = g_api->as_list(args);
  size_t size = g_api->list_size(list);

  if (size < 2) {
    return g_api->create_int(-1);
  }

  sxs_object_t length_obj = g_api->list_at(list, 1);
  sxs_object_t evaled_length = g_api->eval(ctx, length_obj);

  if (g_api->get_type(evaled_length) != SXS_TYPE_INT) {
    return g_api->create_int(-1);
  }

  long long length = g_api->as_int(evaled_length);
  if (length < 0) {
    return g_api->create_int(-1);
  }

  pkg::random::random_string_c gen;
  std::string result = gen.generate_string(static_cast<size_t>(length));

  return g_api->create_string(result.c_str());
}

static sxs_object_t random_string_alpha(sxs_context_t ctx, sxs_object_t args) {
  void *list = g_api->as_list(args);
  size_t size = g_api->list_size(list);

  if (size < 2) {
    return g_api->create_int(-1);
  }

  sxs_object_t length_obj = g_api->list_at(list, 1);
  sxs_object_t evaled_length = g_api->eval(ctx, length_obj);

  if (g_api->get_type(evaled_length) != SXS_TYPE_INT) {
    return g_api->create_int(-1);
  }

  long long length = g_api->as_int(evaled_length);
  if (length < 0) {
    return g_api->create_int(-1);
  }

  pkg::random::random_string_c gen(pkg::random::random_string_c::ALPHA_NUM);
  std::string result = gen.generate_string(static_cast<size_t>(length));

  return g_api->create_string(result.c_str());
}

extern "C" void kernel_init(sxs_registry_t registry,
                            const struct sxs_api_table_t *api) {
  g_api = api;
  api->register_function(registry, "int_range", random_int_range, SXS_TYPE_INT,
                         0);
  api->register_function(registry, "real_range", random_real_range,
                         SXS_TYPE_REAL, 0);
  api->register_function(registry, "string", random_string, SXS_TYPE_STRING, 0);
  api->register_function(registry, "string_alpha", random_string_alpha,
                         SXS_TYPE_STRING, 0);
}
