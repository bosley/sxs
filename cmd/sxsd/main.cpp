#include <runtime/runtime.hpp>
#include <fmt/core.h>
#include <spdlog/spdlog.h>
#include <cstdlib>
#include <optional>
#include <sstream>

void print_usage() {
  fmt::print("Usage: sxsd [options]\n");
  fmt::print("Options:\n");
  fmt::print("  --help, -h\t\t\tPrint this help message\n");
  fmt::print("  --validate-only, -v\t\tValidate the runtime configuration only\n");
  fmt::print("  --runtime-root-path, -r PATH\tSet the runtime root path\n");
  fmt::print("  --include-path, -i PATH\tAdd an include path (can be used multiple times)\n");
  fmt::print("  --event-system-max-threads, -t NUM\tSet the maximum number of event system threads\n");
  fmt::print("  --event-system-max-queue-size, -q NUM\tSet the maximum size of the event system queue\n");
  fmt::print("\nEnvironment Variables:\n");
  fmt::print("  SXSRUNTIME_ROOT_PATH\t\tDefault runtime root path\n");
  fmt::print("  SXSRUNTIME_INCLUDE_PATHS\tColon-separated list of include paths\n");
}

std::optional<std::string> load_from_env(const std::string &env_var) {
  const char *value = std::getenv(env_var.c_str());
  if (value && value[0] != '\0') {
    return std::string(value);
  }
  return std::nullopt;
}

std::vector<std::string> split_paths(const std::string &paths) {
  std::vector<std::string> result;
  std::stringstream ss(paths);
  std::string item;
  while (std::getline(ss, item, ':')) {
    if (!item.empty()) {
      result.push_back(item);
    }
  }
  return result;
}

int main(int argc, char **argv) {
  std::vector<std::string> args(argv, argv + argc);

  if (args.size() == 1) {
    print_usage();
    return 1;
  }

  runtime::options_s options; 

  if (auto runtime_root_path = load_from_env("SXSRUNTIME_ROOT_PATH")) {
    options.runtime_root_path = *runtime_root_path;
  }

  if (auto include_paths_str = load_from_env("SXSRUNTIME_INCLUDE_PATHS")) {
    options.include_paths = split_paths(*include_paths_str);
  }

  if (auto event_system_max_threads = load_from_env("SXSEVENT_SYSTEM_MAX_THREADS")) {
    options.event_system_max_threads = std::stoi(*event_system_max_threads);
  }

  if (auto event_system_max_queue_size = load_from_env("SXSEVENT_SYSTEM_MAX_QUEUE_SIZE")) {
    options.event_system_max_queue_size = std::stoi(*event_system_max_queue_size);
  }

  for (size_t i = 1; i < args.size(); ++i) {
    const auto &arg = args[i];

    if (arg == "--help" || arg == "-h") {
      print_usage();
      return 0;
    } else if (arg == "--validate-only" || arg == "-v") {
      options.validate_only = true;
    } else if (arg == "--runtime-root-path" || arg == "-r") {
      if (i + 1 >= args.size()) {
        fmt::print(stderr, "Error: {} requires a path argument\n", arg);
        return 1;
      }
      options.runtime_root_path = args[++i];
    } else if (arg == "--include-path" || arg == "-i") {
      if (i + 1 >= args.size()) {
        fmt::print(stderr, "Error: {} requires a path argument\n", arg);
        return 1;
      }
      options.include_paths.push_back(args[++i]);
    } else if (arg == "--event-system-max-threads" || arg == "-t") {
      if (i + 1 >= args.size()) {
        fmt::print(stderr, "Error: {} requires a number argument\n", arg);
        return 1;
      }
      options.event_system_max_threads = std::stoi(args[++i]);
    } else if (arg == "--event-system-max-queue-size" || arg == "-q") {
      if (i + 1 >= args.size()) {
        fmt::print(stderr, "Error: {} requires a number argument\n", arg);
        return 1;
      }
      options.event_system_max_queue_size = std::stoi(args[++i]);
    } else {
      fmt::print(stderr, "Error: Unknown option '{}'\n", arg);
      print_usage();
      return 1;
    }
  }

  if (!options.runtime_root_path.empty()) {
    fmt::print("Runtime root path: {}\n", options.runtime_root_path);
  }
  if (!options.include_paths.empty()) {
    fmt::print("Include paths ({}): ", options.include_paths.size());
    for (size_t i = 0; i < options.include_paths.size(); ++i) {
      fmt::print("{}{}", options.include_paths[i], 
                 i < options.include_paths.size() - 1 ? ", " : "\n");
    }
  }

  runtime::runtime_c runtime(options);
  auto logger = runtime.get_logger();

  if (options.validate_only) {
    logger->info("Validating runtime configuration...");
    if (!runtime.initialize()) {
      logger->error("Failed to initialize runtime");
      return 1;
    }
    logger->info("Runtime configuration is valid");
    runtime.shutdown();
    return 0;
  }

  logger->info("Starting SXS daemon...");

  if (!runtime.initialize()) {
    logger->error("Failed to initialize runtime");
    return 1;
  }

  logger->info("Runtime initialized successfully");
  logger->info("Runtime is running: {}", runtime.is_running());

  runtime.shutdown();
  logger->info("Runtime shutdown complete");

  return 0;
}