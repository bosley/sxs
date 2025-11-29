#include "manager.hpp"
#include <filesystem>
#include <fmt/core.h>
#include <vector>

namespace cmd::sxs {

namespace fs = std::filesystem;

void deps(dependency_mgmt_data_s data) {
  fs::path project_path = data.project_dir;

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
    return;
  }

  std::string project_name = project_path.filename().string();

  fmt::print("\n\033["
             "36m╔═════════════════════════════════════════════════════════════"
             "═╗\033[0m\n");
  fmt::print("\033[36m║\033[0m              \033[1mProject Dependencies "
             "Information\033[0m              \033[36m║\033[0m\n");
  fmt::print("\033["
             "36m╚═════════════════════════════════════════════════════════════"
             "═╝\033[0m\n\n");

  fmt::print("\033[1;36m📦 Project Information\033[0m\n");
  fmt::print("  Name: \033[1m{}\033[0m\n", project_name);
  fmt::print("  Path: {}\n\n", project_path.string());

  std::vector<std::string> include_paths;
  fs::path cache_kernels = project_path / ".sxs-cache" / "kernels";
  if (fs::exists(cache_kernels) && fs::is_directory(cache_kernels)) {
    include_paths.push_back(cache_kernels.string());
  }

  const char *sxs_home = std::getenv("SXS_HOME");
  if (sxs_home) {
    fs::path system_kernels = fs::path(sxs_home) / "lib" / "kernels";
    if (fs::exists(system_kernels) && fs::is_directory(system_kernels)) {
      include_paths.push_back(system_kernels.string());
    }
  }

  fmt::print("\033[1;36m🔍 Include Path Order (Priority)\033[0m\n");
  if (include_paths.empty()) {
    fmt::print("  \033[33mNo kernel paths configured\033[0m\n");
  } else {
    for (size_t i = 0; i < include_paths.size(); i++) {
      fmt::print("  {}. {}\n", i + 1, include_paths[i]);
    }
  }
  fmt::print("\n");

  fs::path project_kernels_src = project_path / "kernels";
  fmt::print("\033[1;36m⚙️  Project Kernels\033[0m\n");
  if (!fs::exists(project_kernels_src) ||
      !fs::is_directory(project_kernels_src)) {
    fmt::print("  \033[33mNo kernels directory found\033[0m\n");
  } else {
    bool has_kernels = false;
    fmt::print(
        "  "
        "┌─────────────────────┬──────────┬──────────────────────────────┐\n");
    fmt::print("  │ \033[1mKernel Name\033[0m         │ \033[1mCached\033[0m   "
               "│ \033[1mLibrary\033[0m                      │\n");
    fmt::print(
        "  "
        "├─────────────────────┼──────────┼──────────────────────────────┤\n");

    for (const auto &entry : fs::directory_iterator(project_kernels_src)) {
      if (entry.is_directory()) {
        has_kernels = true;
        std::string kernel_name = entry.path().filename().string();

        fs::path cache_kernel_dir = cache_kernels / kernel_name;
        bool is_cached = false;
        std::string lib_name = "none";

        std::vector<std::string> extensions = {".dylib", ".so"};
        for (const auto &ext : extensions) {
          std::string potential_lib =
              fmt::format("libkernel_{}{}", kernel_name, ext);
          if (fs::exists(cache_kernel_dir / potential_lib)) {
            is_cached = true;
            lib_name = potential_lib;
            break;
          }
        }

        std::string cached_status =
            is_cached ? "\033[32m✓ Yes\033[0m   " : "\033[31m✗ No\033[0m    ";
        fmt::print("  │ {:<19} │ {} │ {:<28} │\n", kernel_name.substr(0, 19),
                   cached_status, lib_name.substr(0, 28));
      }
    }

    if (!has_kernels) {
      fmt::print("  │ \033[33mNo kernels found\033[0m                          "
                 "                 │\n");
    }
    fmt::print(
        "  "
        "└─────────────────────┴──────────┴──────────────────────────────┘\n");
  }
  fmt::print("\n");

  if (sxs_home) {
    fs::path system_kernels = fs::path(sxs_home) / "lib" / "kernels";
    fmt::print("\033[1;36m🌐 System Kernels\033[0m ($SXS_HOME/lib/kernels)\n");
    if (!fs::exists(system_kernels) || !fs::is_directory(system_kernels)) {
      fmt::print("  \033[33mNo system kernels found\033[0m\n");
    } else {
      bool has_system_kernels = false;
      for (const auto &entry : fs::directory_iterator(system_kernels)) {
        if (entry.is_directory()) {
          has_system_kernels = true;
          std::string kernel_name = entry.path().filename().string();
          fmt::print("  • {}\n", kernel_name);
        }
      }
      if (!has_system_kernels) {
        fmt::print("  \033[33mNo system kernels installed\033[0m\n");
      }
    }
  } else {
    fmt::print("\033[1;36m🌐 System Kernels\033[0m\n");
    fmt::print("  \033[33mSXS_HOME not set\033[0m\n");
  }
  fmt::print("\n");

  fs::path modules_dir = project_path / "modules";
  fmt::print("\033[1;36m📚 Modules\033[0m\n");
  if (!fs::exists(modules_dir) || !fs::is_directory(modules_dir)) {
    fmt::print("  \033[33mNo modules directory found\033[0m\n");
  } else {
    int module_count = 0;
    std::vector<std::string> module_names;

    for (const auto &entry : fs::directory_iterator(modules_dir)) {
      if (entry.is_directory()) {
        module_count++;
        module_names.push_back(entry.path().filename().string());
      }
    }

    if (module_count == 0) {
      fmt::print("  \033[33mNo modules found\033[0m\n");
    } else {
      fmt::print("  Total: \033[1m{}\033[0m module(s)\n", module_count);
      for (const auto &name : module_names) {
        fmt::print("  • {}\n", name);
      }
    }
  }
  fmt::print("\n");

  fs::path cache_dir = project_path / ".sxs-cache";
  fmt::print("\033[1;36m💾 Cache Status\033[0m\n");
  if (!fs::exists(cache_dir)) {
    fmt::print("  \033[33mNo cache directory\033[0m\n");
  } else {
    int cached_libs = 0;
    std::uintmax_t total_size = 0;

    if (fs::exists(cache_kernels)) {
      for (const auto &kernel_entry : fs::directory_iterator(cache_kernels)) {
        if (kernel_entry.is_directory()) {
          for (const auto &file_entry :
               fs::directory_iterator(kernel_entry.path())) {
            if (file_entry.is_regular_file()) {
              auto ext = file_entry.path().extension();
              if (ext == ".dylib" || ext == ".so") {
                cached_libs++;
                total_size += fs::file_size(file_entry.path());
              }
            }
          }
        }
      }
    }

    fmt::print("  Location: {}\n", cache_dir.string());
    fmt::print("  Cached libraries: \033[1m{}\033[0m\n", cached_libs);

    double size_kb = total_size / 1024.0;
    double size_mb = size_kb / 1024.0;
    if (size_mb >= 1.0) {
      fmt::print("  Total size: \033[1m{:.2f} MB\033[0m\n", size_mb);
    } else {
      fmt::print("  Total size: \033[1m{:.2f} KB\033[0m\n", size_kb);
    }
  }
  fmt::print("\n");
}

} // namespace cmd::sxs
