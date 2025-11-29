// this is the main runtime server of sxs. this should be installed and running
// on a target machine to get a full-featured instance of the sup and full
// app execution sxs runs either scripts OR applications. The applications
// _require_ the sup to be present and running

#include "manager.hpp"
#include <fmt/core.h>
#include <vector>

void usage() {
  fmt::print("Usage: sup <command> [options]\n");
  fmt::print("Commands:\n");
  fmt::print("  build-info          Show the build information\n");
  fmt::print("  build <project dir> Build project kernels (default './')\n");
  fmt::print("  run <project dir>   Run a project (default './')\n");
  fmt::print(
      "  new <project name> <dir> (default './')   Create a new project\n");
  fmt::print(
      "  deps <project dir>  Show project dependencies (default './')\n");
  fmt::print("  clean <project dir> Clean project cache (default './')\n");
  fmt::print("  help                Show this help message\n");
}

void build_info() { fmt::print("Build hash > {}\n", BUILD_HASH); }

#define EITHER(arg, command_short, command_long)                               \
  (arg == command_short || arg == command_long)

#define FLAG_IS(command_short, command_long, body)                             \
  for (const auto &arg : args) {                                               \
    if (EITHER(arg, command_short, command_long)) {                            \
      body;                                                                    \
      return 0;                                                                \
    }                                                                          \
  }

#define COMMAND_IS(command_str, body)                                          \
  if (args[1] == command_str) {                                                \
    body;                                                                      \
    return 0;                                                                  \
  }

void new_project(std::vector<std::string> args);
void deps(std::vector<std::string> args);
void build(std::vector<std::string> args);
void run(std::vector<std::string> args);
void clean(std::vector<std::string> args);

int main(int argc, char **argv) {
  auto args = std::vector<std::string>(argv, argv + argc);

  FLAG_IS("-h", "--help", { usage(); });

  FLAG_IS("-bi", "--build-info", { build_info(); });

  COMMAND_IS("new", { new_project(args); });

  COMMAND_IS("deps", { deps(args); });

  COMMAND_IS("build", { build(args); });

  COMMAND_IS("run", { run(args); });

  COMMAND_IS("clean", { clean(args); });

  if (args.size() < 2) {
    usage();
    return 1;
  }
}

void new_project(std::vector<std::string> args) {
  cmd::sxs::project_mgmt_data_s data;
  if (args.size() > 2) {
    data.project_name = args[2];
  }
  if (args.size() > 3) {
    data.project_dir = args[3];
  } else {
    data.project_dir = "./";
  }
  cmd::sxs::new_project(data);
}

void deps(std::vector<std::string> args) {
  cmd::sxs::dependency_mgmt_data_s data;
  if (args.size() > 2) {
    data.project_dir = args[2];
  } else {
    data.project_dir = "./";
  }
  cmd::sxs::deps(data);
}

void clean(std::vector<std::string> args) {
  std::string project_dir;
  if (args.size() > 2) {
    project_dir = args[2];
  } else {
    project_dir = "./";
  }
  cmd::sxs::clean(project_dir);
}

void build(std::vector<std::string> args) {
  cmd::sxs::runtime_setup_data_s data;
  if (args.size() > 2) {
    data.project_dir = args[2];
  } else {
    data.project_dir = "./";
  }
  cmd::sxs::build(data);
}

void run(std::vector<std::string> args) {
  cmd::sxs::runtime_setup_data_s data;
  if (args.size() > 2) {
    data.project_dir = args[2];
  } else {
    data.project_dir = "./";
  }
  cmd::sxs::run(data);
}