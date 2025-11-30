#include <cstdio>
#include <cstring>
#include <kernel_api.hpp>
#include <string>
#include <vector>

static const struct pkg::kernel::api_table_s *g_api = nullptr;

static slp::slp_object_c io_put(pkg::kernel::context_t ctx,
                                const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 2) {
    return g_api->create_int(-1);
  }

  auto evaled_format = g_api->eval(ctx, list.at(1));
  if (evaled_format.type() != slp::slp_type_e::DQ_LIST) {
    return g_api->create_int(-1);
  }

  std::string format = evaled_format.as_string().to_string();

  std::vector<slp::slp_object_c> evaled_args;
  for (size_t i = 2; i < list.size(); i++) {
    evaled_args.push_back(g_api->eval(ctx, list.at(i)));
  }

  std::string output;
  size_t arg_index = 0;
  for (size_t i = 0; i < format.length(); i++) {
    if (format[i] == '%' && i + 1 < format.length()) {
      char specifier = format[i + 1];
      if (arg_index < evaled_args.size()) {
        const auto &arg = evaled_args[arg_index];
        auto type = arg.type();

        if (specifier == 'd' && type == slp::slp_type_e::INTEGER) {
          output += std::to_string(arg.as_int());
          i++;
          arg_index++;
        } else if (specifier == 'f' && type == slp::slp_type_e::REAL) {
          char buf[64];
          snprintf(buf, sizeof(buf), "%f", arg.as_real());
          output += buf;
          i++;
          arg_index++;
        } else if (specifier == 's' && type == slp::slp_type_e::DQ_LIST) {
          output += arg.as_string().to_string();
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

extern "C" void kernel_init(pkg::kernel::registry_t registry,
                            const struct pkg::kernel::api_table_s *api) {
  g_api = api;
  api->register_function(registry, "put", io_put, slp::slp_type_e::INTEGER, 1);
}
