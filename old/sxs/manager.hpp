#pragma once

#include <string>

namespace cmd::sxs {

struct project_mgmt_data_s {
  std::string project_name;
  std::string project_dir;
};

struct dependency_mgmt_data_s {
  std::string project_dir;
};

struct runtime_setup_data_s {
  std::string project_dir;
};

void new_project(project_mgmt_data_s data);
void deps(dependency_mgmt_data_s data);
void build(runtime_setup_data_s data);
void run(runtime_setup_data_s data);
void clean(std::string project_dir);

} // namespace cmd::sxs
