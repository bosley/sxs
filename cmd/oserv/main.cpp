#include <fmt/core.h>

#include "config.hpp"

void print_usage() {
  fmt::print("Usage: oserv <config_file>\n");
  fmt::print("Options:\n");
  fmt::print("  --help, -h\t\tPrint this help message\n");
  fmt::print("  --new-config, -n\tCreate a new config file\n");
}

int main(int argc, char **argv) {

  std::vector<std::string> args(argv, argv + argc);

  if (args.size() == 1) {
    print_usage();
    return 0;
  }

  for (auto i = 1; i < args.size(); i++) {
    if (args[i] == "--help" || args[i] == "-h") {
      print_usage();
      return 0;
    }

    if (args[i] == "--new-config" || args[i] == "-n") {
      if (app::new_config("config.json")) {
        fmt::print("Created new config file: config.json\n");
        return 0;
      } else {
        fmt::print("Failed to create new config file: config.json\n");
        return 1;
      }
    }
  }

  app::config_c config;
  if (!app::load_config(argv[1], config)) {
    fmt::print("Failed to load config file: {}\n", argv[1]);
    return 1;
  }
  fmt::print("oserv\n");
  fmt::print("config: {}\n", config.get_http_address());
  fmt::print("config: {}\n", config.get_http_port());
  fmt::print("config: {}\n", config.get_http_threads());
  fmt::print("config: {}\n", config.get_http_max_connections());

  if (!config.ensure_data_dir()) {
    fmt::print("Failed to ensure data directory: {}\n", config.get_data_path());
    return 1;
  }

  fmt::print("data directory: {}\n", config.get_data_path());

  return 0;
}