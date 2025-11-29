#include "manager.hpp"
#include <filesystem>
#include <fmt/core.h>

namespace cmd::sxs {

namespace fs = std::filesystem;

void clean(std::string project_dir) {
  fs::path project_path = project_dir;

  if (!project_path.is_absolute()) {
    project_path = fs::absolute(project_path);
  }

  if (!fs::exists(project_path)) {
    fmt::print("Error: Directory '{}' does not exist\n", project_path.string());
    return;
  }

  if (!fs::is_directory(project_path)) {
    fmt::print("Error: '{}' is not a directory\n", project_path.string());
    return;
  }

  fs::path init_file = project_path / "init.sxs";
  if (!fs::exists(init_file)) {
    fmt::print("Error: Not a valid project directory (missing init.sxs)\n");
    fmt::print("Directory: {}\n", project_path.string());
    return;
  }

  fs::path cache_dir = project_path / ".sxs-cache";

  if (!fs::exists(cache_dir)) {
    fmt::print("No cache to clean in: {}\n", project_path.string());
    return;
  }

  try {
    std::uintmax_t removed_count = fs::remove_all(cache_dir);
    fmt::print("\033[32m✓\033[0m Cleaned cache from project: {}\n",
               project_path.string());
    fmt::print("  Removed {} items\n", removed_count);
  } catch (const std::exception &e) {
    fmt::print("\033[31m✗\033[0m Failed to clean cache: {}\n", e.what());
  }
}

} // namespace cmd::sxs
