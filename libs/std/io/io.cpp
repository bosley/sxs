#include "kernel_api.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

static const struct sxs_api_table_t *g_api = nullptr;

static sxs_object_t io_put(sxs_context_t ctx, sxs_object_t args) {
  void *list = g_api->as_list(args);
  size_t size = g_api->list_size(list);

  if (size < 2) {
    return g_api->create_int(-1);
  }

  sxs_object_t format_obj = g_api->list_at(list, 1);
  sxs_object_t evaled_format = g_api->eval(ctx, format_obj);
  sxs_type_t type = g_api->get_type(evaled_format);

  if (type != SXS_TYPE_STRING) {
    return g_api->create_int(-1);
  }

  const char *format_str = g_api->as_string(evaled_format);
  std::string format = format_str ? format_str : "";

  std::vector<sxs_object_t> evaled_args;
  for (size_t i = 2; i < size; i++) {
    sxs_object_t arg = g_api->list_at(list, i);
    sxs_object_t evaled = g_api->eval(ctx, arg);
    evaled_args.push_back(evaled);
  }

  std::string output;
  size_t arg_index = 0;
  for (size_t i = 0; i < format.length(); i++) {
    if (format[i] == '%' && i + 1 < format.length()) {
      char specifier = format[i + 1];
      if (arg_index < evaled_args.size()) {
        sxs_object_t arg = evaled_args[arg_index];
        sxs_type_t type = g_api->get_type(arg);

        if (specifier == 'd' && type == SXS_TYPE_INT) {
          long long val = g_api->as_int(arg);
          output += std::to_string(val);
          i++;
          arg_index++;
        } else if (specifier == 'f' && type == SXS_TYPE_REAL) {
          double val = g_api->as_real(arg);
          char buf[64];
          snprintf(buf, sizeof(buf), "%f", val);
          output += buf;
          i++;
          arg_index++;
        } else if (specifier == 's' && type == SXS_TYPE_STRING) {
          const char *str = g_api->as_string(arg);
          output += str ? str : "";
          i++;
          arg_index++;
        } else {
          output += format[i];
        }
      } else {
        output += format[i];
      }
    } else {
      output += format[i];
    }
  }

  printf("%s", output.c_str());
  fflush(stdout);

  return g_api->create_int(static_cast<long long>(output.length()));
}

extern "C" void kernel_init(sxs_registry_t registry,
                            const struct sxs_api_table_t *api) {
  g_api = api;
  api->register_function(registry, "put", io_put, SXS_TYPE_INT, 1);
}
