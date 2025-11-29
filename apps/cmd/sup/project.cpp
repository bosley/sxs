#include "data.hpp"
#include "manager.hpp"
#include <filesystem>
#include <fmt/core.h>
#include <fstream>

namespace cmd::sxs {

void new_project(project_mgmt_data_s data) {
  namespace fs = std::filesystem;

  if (data.project_name.empty()) {
    fmt::print("Error: Project name cannot be empty\n");
    return;
  }

  fs::path base_dir = data.project_dir;
  fs::path project_path = base_dir / data.project_name;

  if (fs::exists(project_path)) {
    fmt::print("Error: Project directory '{}' already exists\n",
               project_path.string());
    return;
  }

  try {
    fs::create_directories(project_path / "kernels" / data.project_name);
    fs::create_directories(project_path / "modules" / "hello_world");

    auto write_file = [&](const fs::path &path, const std::string &content) {
      std::ofstream file(path);
      if (!file) {
        throw std::runtime_error("Failed to create file: " + path.string());
      }
      file << content;
      file.close();
    };

    std::string kernel_cpp = templates::replace_placeholder(
        templates::EXAMPLE_KERNEL_CPP, "{PROJECT_NAME}", data.project_name);
    write_file(project_path / "kernels" / data.project_name /
                   (data.project_name + ".cpp"),
               kernel_cpp);

    std::string kernel_sxs = templates::replace_placeholder(
        templates::KERNEL_SXS, "{PROJECT_NAME}", data.project_name);
    write_file(project_path / "kernels" / data.project_name / "kernel.sxs",
               kernel_sxs);

    std::string kernel_makefile = templates::replace_placeholder(
        templates::KERNEL_MAKEFILE, "{PROJECT_NAME}", data.project_name);
    write_file(project_path / "kernels" / data.project_name / "Makefile",
               kernel_makefile);

    std::string init_sxs = templates::replace_placeholder(
        templates::INIT_SXS, "{PROJECT_NAME}", data.project_name);
    write_file(project_path / "init.sxs", init_sxs);

    std::string hello_module = templates::replace_placeholder(
        templates::HELLO_WORLD_MODULE, "{PROJECT_NAME}", data.project_name);
    write_file(project_path / "modules" / "hello_world" / "hello_world.sxs",
               hello_module);

    write_file(project_path / ".gitignore", templates::GITIGNORE);

    fmt::print("✓ Successfully created project: {}\n", data.project_name);
    fmt::print("\nProject structure:\n");
    fmt::print("  {}/\n", project_path.string());
    fmt::print("    ├── .gitignore\n");
    fmt::print("    ├── init.sxs\n");
    fmt::print("    ├── kernels/\n");
    fmt::print("    │   └── {}/\n", data.project_name);
    fmt::print("    │       ├── kernel.sxs\n");
    fmt::print("    │       ├── {}.cpp\n", data.project_name);
    fmt::print("    │       └── Makefile\n");
    fmt::print("    └── modules/\n");
    fmt::print("        └── hello_world/\n");
    fmt::print("            └── hello_world.sxs\n");
    fmt::print("\nNext steps:\n");
    fmt::print("  1. cd {}\n", project_path.string());
    fmt::print("  2. cd kernels/{} && make\n", data.project_name);
    fmt::print("  3. sxs init.sxs\n");

  } catch (const std::exception &e) {
    fmt::print("Error creating project: {}\n", e.what());
  }
}

} // namespace cmd::sxs
