#include <core/core.hpp>
#include <filesystem>
#include <iostream>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <file.slp> [options]" << std::endl;
    std::cerr << "Options:" << std::endl;
    std::cerr << "  -w, --working-dir <path>   Set working directory"
              << std::endl;
    std::cerr << "  -i, --include <path>       Add include path (repeatable)"
              << std::endl;
    return 1;
  }

  std::string file_path = argv[1];
  std::string working_directory = std::filesystem::current_path().string();
  std::vector<std::string> include_paths;

  for (int i = 2; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "-w" || arg == "--working-dir") {
      if (i + 1 < argc) {
        working_directory = argv[++i];
      }
    } else if (arg == "-i" || arg == "--include") {
      if (i + 1 < argc) {
        include_paths.push_back(argv[++i]);
      }
    }
  }

  if (!std::filesystem::path(file_path).is_absolute()) {
    file_path = std::filesystem::absolute(file_path).string();
  }

  auto logger = spdlog::stdout_color_mt("sxs");
  logger->set_level(spdlog::level::info);

  pkg::core::option_s options{.file_path = file_path,
                              .include_paths = include_paths,
                              .working_directory = working_directory,
                              .logger = logger};

  try {
    pkg::core::core_c core(options);
    return core.run();
  } catch (const std::exception &e) {
    logger->error("Fatal error: {}", e.what());
    return 1;
  }
}