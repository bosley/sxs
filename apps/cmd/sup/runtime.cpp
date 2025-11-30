#include "manager.hpp"
#include <core/core.hpp>
#include <core/tcs/tcs.hpp>
#include <filesystem>
#include <fmt/core.h>
#include <fstream>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <sstream>

namespace cmd::sxs {

namespace fs = std::filesystem;

static std::string compute_file_hash(const fs::path &file_path) {
  std::ifstream file(file_path, std::ios::binary);
  if (!file) {
    return "";
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string content = buffer.str();

  std::hash<std::string> hasher;
  return std::to_string(hasher(content));
}

static std::string compute_kernel_hash(const fs::path &kernel_dir) {
  std::string combined_hash;

  for (const auto &entry : fs::directory_iterator(kernel_dir)) {
    if (entry.is_regular_file()) {
      auto ext = entry.path().extension();
      if (ext == ".cpp" || ext == ".hpp" || ext == ".h" || ext == "Makefile") {
        combined_hash += compute_file_hash(entry.path());
      }
    }
  }

  std::hash<std::string> hasher;
  return std::to_string(hasher(combined_hash));
}

static std::string read_cached_hash(const fs::path &cache_kernel_dir) {
  fs::path hash_file = cache_kernel_dir / ".build_hash";
  if (!fs::exists(hash_file)) {
    return "";
  }

  std::ifstream file(hash_file);
  if (!file) {
    return "";
  }

  std::string hash;
  std::getline(file, hash);
  return hash;
}

static void write_hash(const fs::path &cache_kernel_dir,
                       const std::string &hash) {
  fs::path hash_file = cache_kernel_dir / ".build_hash";
  std::ofstream file(hash_file);
  if (file) {
    file << hash;
  }
}

static bool build_kernel(const fs::path &kernel_src_dir,
                         const std::string &kernel_name) {
  fmt::print("Building kernel '{}'...\n", kernel_name);

  std::string command =
      fmt::format("cd '{}' && make clean && make", kernel_src_dir.string());
  int result = std::system(command.c_str());

  if (result != 0) {
    fmt::print("  ✗ Build failed with exit code: {}\n", result);
    return false;
  }

  fmt::print("  ✓ Build successful\n");
  return true;
}

static bool find_and_copy_dylib(const fs::path &kernel_src_dir,
                                const fs::path &cache_kernel_dir,
                                const std::string &kernel_name) {
  std::vector<std::string> extensions = {".dylib", ".so"};

  for (const auto &ext : extensions) {
    std::string lib_name = fmt::format("libkernel_{}{}", kernel_name, ext);
    fs::path src_lib = kernel_src_dir / lib_name;

    if (fs::exists(src_lib)) {
      fs::path dest_lib = cache_kernel_dir / lib_name;
      fs::copy_file(src_lib, dest_lib, fs::copy_options::overwrite_existing);
      fmt::print("  ✓ Copied {} to cache\n", lib_name);
      return true;
    }
  }

  fmt::print("  ✗ No built library found\n");
  return false;
}

static bool has_cached_dylib(const fs::path &cache_kernel_dir,
                             const std::string &kernel_name) {
  std::vector<std::string> extensions = {".dylib", ".so"};

  for (const auto &ext : extensions) {
    std::string lib_name = fmt::format("libkernel_{}{}", kernel_name, ext);
    if (fs::exists(cache_kernel_dir / lib_name)) {
      return true;
    }
  }
  return false;
}

static bool process_kernel(const fs::path &kernel_src_dir,
                           const fs::path &cache_dir,
                           const std::string &kernel_name) {
  fs::path cache_kernel_dir = cache_dir / kernel_name;
  fs::create_directories(cache_kernel_dir);

  std::string current_hash = compute_kernel_hash(kernel_src_dir);
  std::string cached_hash = read_cached_hash(cache_kernel_dir);

  if (current_hash == cached_hash && !cached_hash.empty() &&
      has_cached_dylib(cache_kernel_dir, kernel_name)) {
    fmt::print("Kernel '{}' is up to date\n", kernel_name);
    return true;
  }

  if (current_hash != cached_hash) {
    fmt::print("Kernel '{}' source changed, rebuilding...\n", kernel_name);
  } else {
    fmt::print("Kernel '{}' has no cached build, building...\n", kernel_name);
  }

  bool build_success = build_kernel(kernel_src_dir, kernel_name);

  if (build_success) {
    bool copy_success =
        find_and_copy_dylib(kernel_src_dir, cache_kernel_dir, kernel_name);
    if (copy_success) {
      write_hash(cache_kernel_dir, current_hash);
      return true;
    }
  }

  if (has_cached_dylib(cache_kernel_dir, kernel_name)) {
    fmt::print("Build failed, but using cached library from previous build\n");
    return true;
  }

  fmt::print("  ✗ No usable kernel library available for '{}'\n", kernel_name);
  return false;
}

static bool check_project_types(const fs::path &project_path) {
  fs::path init_file = project_path / "init.sxs";

  fs::path cache_dir = project_path / ".sxs-cache" / "kernels";
  std::vector<std::string> include_paths;

  if (fs::exists(cache_dir) && fs::is_directory(cache_dir)) {
    include_paths.push_back(cache_dir.string());
  }

  const char *sxs_home = std::getenv("SXS_HOME");
  if (sxs_home) {
    fs::path system_kernels = fs::path(sxs_home) / "lib" / "kernels";
    if (fs::exists(system_kernels) && fs::is_directory(system_kernels)) {
      include_paths.push_back(system_kernels.string());
    }
  }

  auto logger = spdlog::stdout_color_mt("tcs");
  logger->set_level(spdlog::level::info);

  pkg::core::tcs::tcs_c type_checker(logger, include_paths,
                                     project_path.string());

  fmt::print("\n=== Validating Project (Types & Symbols) ===\n");
  bool success = type_checker.check(init_file.string());

  if (success) {
    fmt::print("  ✓ Validation passed\n");
  } else {
    fmt::print("  ✗ Validation failed\n");
  }
  fmt::print("\n");

  spdlog::drop("tcs");
  return success;
}

static bool build_project_kernels(const fs::path &project_path) {
  fs::path cache_dir = project_path / ".sxs-cache" / "kernels";
  fs::create_directories(cache_dir);

  fs::path project_kernels_src = project_path / "kernels";
  if (!fs::exists(project_kernels_src) ||
      !fs::is_directory(project_kernels_src)) {
    fmt::print("No kernels directory found in project\n");
    return true;
  }

  fmt::print("\n=== Processing Project Kernels ===\n");
  bool all_success = true;

  for (const auto &entry : fs::directory_iterator(project_kernels_src)) {
    if (entry.is_directory()) {
      std::string kernel_name = entry.path().filename().string();

      if (!process_kernel(entry.path(), cache_dir, kernel_name)) {
        fmt::print("Warning: Kernel '{}' could not be built or cached\n",
                   kernel_name);
        all_success = false;
      }
    }
  }
  fmt::print("\n");
  return all_success;
}

void build(runtime_setup_data_s data) {
  fs::path project_path = data.project_dir;

  if (!project_path.is_absolute()) {
    project_path = fs::absolute(project_path);
  }

  if (!fs::exists(project_path)) {
    fmt::print("Error: Project directory '{}' does not exist\n",
               project_path.string());
    return;
  }

  if (!fs::is_directory(project_path)) {
    fmt::print("Error: '{}' is not a directory\n", project_path.string());
    return;
  }

  fs::path init_file = project_path / "init.sxs";
  if (!fs::exists(init_file)) {
    fmt::print("Error: init.sxs not found in project directory '{}'\n",
               project_path.string());
    return;
  }

  bool type_check_success = check_project_types(project_path);
  if (!type_check_success) {
    fmt::print("\033[31m✗\033[0m Build failed: Validation errors\n");
    return;
  }

  bool kernel_success = build_project_kernels(project_path);

  if (kernel_success) {
    fmt::print("\033[32m✓\033[0m Build completed successfully\n");
  } else {
    fmt::print("\033[33m⚠\033[0m Build completed with warnings\n");
  }
}

void run(runtime_setup_data_s data) {
  fs::path project_path = data.project_dir;

  if (!project_path.is_absolute()) {
    project_path = fs::absolute(project_path);
  }

  if (!fs::exists(project_path)) {
    fmt::print("Error: Project directory '{}' does not exist\n",
               project_path.string());
    return;
  }

  if (!fs::is_directory(project_path)) {
    fmt::print("Error: '{}' is not a directory\n", project_path.string());
    return;
  }

  fs::path init_file = project_path / "init.sxs";
  if (!fs::exists(init_file)) {
    fmt::print("Error: init.sxs not found in project directory '{}'\n",
               project_path.string());
    return;
  }

  bool type_check_success = check_project_types(project_path);
  if (!type_check_success) {
    fmt::print("\033[31m✗\033[0m Validation failed, aborting run\n");
    return;
  }

  build_project_kernels(project_path);

  fs::path cache_dir = project_path / ".sxs-cache" / "kernels";
  std::vector<std::string> include_paths;

  if (fs::exists(cache_dir) && fs::is_directory(cache_dir)) {
    include_paths.push_back(cache_dir.string());
  }

  const char *sxs_home = std::getenv("SXS_HOME");
  if (sxs_home) {
    fs::path system_kernels = fs::path(sxs_home) / "lib" / "kernels";
    if (fs::exists(system_kernels) && fs::is_directory(system_kernels)) {
      include_paths.push_back(system_kernels.string());
    }
  }

  auto logger = spdlog::stdout_color_mt("sup");
  logger->set_level(spdlog::level::info);

  pkg::core::option_s options{.file_path = init_file.string(),
                              .include_paths = include_paths,
                              .working_directory = project_path.string(),
                              .logger = logger};

  fmt::print("=== Running Project ===\n");
  try {
    pkg::core::core_c core(options);
    int result = core.run();
    if (result != 0) {
      fmt::print("\nProject execution completed with exit code: {}\n", result);
    }
  } catch (const std::exception &e) {
    logger->error("Fatal error: {}", e.what());
    fmt::print("Error running project: {}\n", e.what());
  }
}

} // namespace cmd::sxs
