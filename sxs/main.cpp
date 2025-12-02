#include "manager.hpp"
#include <core/core.hpp>
#include <filesystem>
#include <fmt/core.h>
#include <iostream>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <vector>

namespace fs = std::filesystem;

void usage() {
  fmt::print("SXS - SXS Language Runtime and Meta-Compiler\n\n");
  fmt::print("Usage:\n");
  fmt::print("  sxs [options] <file.sxs>              Run a script\n");
  fmt::print("  sxs <command> [options] [args]        Run a command\n\n");
  fmt::print("Script Options:\n");
  fmt::print("  -w, --working-dir <path>   Set working directory\n");
  fmt::print("  -i, --include <path>       Add include path (repeatable)\n");
  fmt::print("  -v, --verbose              Enable verbose logging\n");
  fmt::print(
      "  -q, --quiet                Suppress all output except errors\n");
  fmt::print("  -l, --log-level <level>    Set log level (trace, debug, info, "
             "warn, error, critical)\n\n");
  fmt::print("Commands:\n");
  fmt::print("  project new <name> [dir]   Create a new project\n");
  fmt::print("  project build [dir]        Build project kernels\n");
  fmt::print("  project run [dir]          Build and run project\n");
  fmt::print("  project clean [dir]        Clean project cache\n");
  fmt::print("  deps [dir]                 Show project dependencies\n");
  fmt::print("  check <file|dir>           Type check code (stub)\n");
  fmt::print("  test [dir]                 Run tests (stub)\n");
  fmt::print("  compile <file> -o <out>    Compile program (stub)\n");
  fmt::print("  kernel list [dir]          List kernels (stub)\n");
  fmt::print("  kernel info <name>         Show kernel info (stub)\n");
  fmt::print("  kernel build <name>        Build kernel (stub)\n");
  fmt::print("  version                    Show version info\n");
  fmt::print("  info [dir]                 Show runtime info (stub)\n");
  fmt::print("  help                       Show this help message\n");
}

void version() {
  fmt::print("SXS Version 1.0.0\n");
  fmt::print("Build hash: {}\n", BUILD_HASH);
  fmt::print("Platform: {}\n",
#ifdef __APPLE__
             "macOS"
#elif __linux__
             "Linux"
#else
             "Unknown"
#endif
  );
}

int run_script(int argc, char **argv, int start_idx) {
  if (start_idx >= argc) {
    fmt::print("Error: No script file specified\n");
    return 2;
  }

  std::string file_path = argv[start_idx];
  std::string working_directory = fs::current_path().string();
  std::vector<std::string> include_paths;
  spdlog::level::level_enum log_level = spdlog::level::info;

  for (int i = start_idx + 1; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "-w" || arg == "--working-dir") {
      if (i + 1 < argc) {
        working_directory = argv[++i];
      }
    } else if (arg == "-i" || arg == "--include") {
      if (i + 1 < argc) {
        include_paths.push_back(argv[++i]);
      }
    } else if (arg == "-v" || arg == "--verbose") {
      log_level = spdlog::level::debug;
    } else if (arg == "-q" || arg == "--quiet") {
      log_level = spdlog::level::err;
    } else if (arg == "-l" || arg == "--log-level") {
      if (i + 1 < argc) {
        std::string level_str = argv[++i];
        if (level_str == "trace")
          log_level = spdlog::level::trace;
        else if (level_str == "debug")
          log_level = spdlog::level::debug;
        else if (level_str == "info")
          log_level = spdlog::level::info;
        else if (level_str == "warn")
          log_level = spdlog::level::warn;
        else if (level_str == "error")
          log_level = spdlog::level::err;
        else if (level_str == "critical")
          log_level = spdlog::level::critical;
      }
    }
  }

  if (!fs::path(file_path).is_absolute()) {
    file_path = fs::absolute(file_path).string();
  }

  const char *sxs_home = std::getenv("SXS_HOME");
  if (sxs_home) {
    fs::path kernel_path = fs::path(sxs_home) / "lib" / "kernels";
    if (fs::exists(kernel_path)) {
      std::string kernel_path_str = kernel_path.string();
      bool already_added = false;
      for (const auto &path : include_paths) {
        if (fs::equivalent(path, kernel_path_str)) {
          already_added = true;
          break;
        }
      }
      if (!already_added) {
        include_paths.push_back(kernel_path_str);
      }
    }
  }

  auto logger = spdlog::stdout_color_mt("sxs");
  logger->set_level(log_level);

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

void stub_command(const std::string &command) {
  fmt::print("TODO: Command '{}' not yet implemented\n", command);
  fmt::print("This is a stub. Full implementation coming soon.\n");
}

int main(int argc, char **argv) {
  auto args = std::vector<std::string>(argv, argv + argc);

  if (argc < 2) {
    usage();
    return 1;
  }

  std::string first_arg = args[1];

  if (first_arg == "-h" || first_arg == "--help" || first_arg == "help") {
    usage();
    return 0;
  }

  if (first_arg == "version") {
    version();
    return 0;
  }

  if (first_arg == "project") {
    if (argc < 3) {
      fmt::print("Error: 'project' requires a subcommand\n");
      fmt::print("Available: new, build, run, clean\n");
      return 2;
    }

    std::string subcmd = args[2];

    if (subcmd == "new") {
      cmd::sxs::project_mgmt_data_s data;
      if (argc > 3) {
        data.project_name = args[3];
      } else {
        fmt::print("Error: 'project new' requires a project name\n");
        return 2;
      }
      if (argc > 4) {
        data.project_dir = args[4];
      } else {
        data.project_dir = "./";
      }
      cmd::sxs::new_project(data);
      return 0;
    }

    if (subcmd == "build") {
      cmd::sxs::runtime_setup_data_s data;
      if (argc > 3) {
        data.project_dir = args[3];
      } else {
        data.project_dir = "./";
      }
      cmd::sxs::build(data);
      return 0;
    }

    if (subcmd == "run") {
      cmd::sxs::runtime_setup_data_s data;
      if (argc > 3) {
        data.project_dir = args[3];
      } else {
        data.project_dir = "./";
      }
      cmd::sxs::run(data);
      return 0;
    }

    if (subcmd == "clean") {
      std::string project_dir;
      if (argc > 3) {
        project_dir = args[3];
      } else {
        project_dir = "./";
      }
      cmd::sxs::clean(project_dir);
      return 0;
    }

    fmt::print("Error: Unknown project subcommand '{}'\n", subcmd);
    return 2;
  }

  if (first_arg == "deps") {
    cmd::sxs::dependency_mgmt_data_s data;
    if (argc > 2) {
      data.project_dir = args[2];
    } else {
      data.project_dir = "./";
    }
    cmd::sxs::deps(data);
    return 0;
  }

  if (first_arg == "check") {
    stub_command("check");
    return 0;
  }

  if (first_arg == "test") {
    stub_command("test");
    return 0;
  }

  if (first_arg == "compile") {
    stub_command("compile");
    return 0;
  }

  if (first_arg == "kernel") {
    if (argc < 3) {
      fmt::print("Error: 'kernel' requires a subcommand\n");
      fmt::print("Available: list, info, build\n");
      return 2;
    }
    stub_command(fmt::format("kernel {}", args[2]));
    return 0;
  }

  if (first_arg == "info") {
    stub_command("info");
    return 0;
  }

  if (first_arg == "run") {
    return run_script(argc, argv, 2);
  }

  if (first_arg[0] == '-' || fs::exists(first_arg)) {
    return run_script(argc, argv, 1);
  }

  fmt::print("Error: Unknown command or file '{}'\n", first_arg);
  fmt::print("Run 'sxs help' for usage information\n");
  return 2;
}
