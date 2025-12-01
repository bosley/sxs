#pragma once

#include "core/core.hpp"
#include <map>
#include <set>
#include <string>
#include <sxs/slp/slp.hpp>
#include <vector>

namespace pkg::core::tcs {

struct type_info_s {
  slp::slp_type_e base_type;
  std::string lambda_signature;
  bool is_variadic{false};
  std::uint64_t lambda_id{0};
};

struct function_signature_s {
  std::vector<type_info_s> parameters;
  type_info_s return_type;
  bool variadic{false};
};

class tcs_c {
public:
  explicit tcs_c(logger_t logger, std::vector<std::string> include_paths,
                 std::string working_directory);
  ~tcs_c();

  bool check(const std::string &file_path);

  bool check_source(const std::string &source,
                    const std::string &source_name = "<string>");

private:
  type_info_s eval_type(slp::slp_object_c &object);

  type_info_s handle_def(slp::slp_object_c &args_list);
  type_info_s handle_fn(slp::slp_object_c &args_list);
  type_info_s handle_if(slp::slp_object_c &args_list);
  type_info_s handle_match(slp::slp_object_c &args_list);
  type_info_s handle_reflect(slp::slp_object_c &args_list);
  type_info_s handle_try(slp::slp_object_c &args_list);
  type_info_s handle_recover(slp::slp_object_c &args_list);
  type_info_s handle_assert(slp::slp_object_c &args_list);
  type_info_s handle_eval(slp::slp_object_c &args_list);
  type_info_s handle_apply(slp::slp_object_c &args_list);
  type_info_s handle_export(slp::slp_object_c &args_list);
  type_info_s handle_debug(slp::slp_object_c &args_list);
  type_info_s handle_cast(slp::slp_object_c &args_list);
  type_info_s handle_do(slp::slp_object_c &args_list);
  type_info_s handle_done(slp::slp_object_c &args_list);

  type_info_s handle_import(slp::slp_object_c &args_list);
  type_info_s handle_load(slp::slp_object_c &args_list);

  void push_scope();
  void pop_scope();
  bool has_symbol(const std::string &symbol, bool local_scope_only = false);
  void define_symbol(const std::string &symbol, const type_info_s &type);
  type_info_s get_symbol_type(const std::string &symbol);

  bool is_type_symbol(const std::string &symbol, type_info_s &out_type);

  std::string resolve_file_path(const std::string &file_path);
  std::string resolve_kernel_path(const std::string &kernel_name);
  bool load_kernel_types(const std::string &kernel_name,
                         const std::string &kernel_dir);

  bool types_match(const type_info_s &expected, const type_info_s &actual);

  logger_t logger_;
  std::vector<std::string> include_paths_;
  std::string working_directory_;
  std::vector<std::map<std::string, type_info_s>> scopes_;
  std::map<std::string, type_info_s> type_symbol_map_;
  std::map<std::string, function_signature_s> function_signatures_;
  std::map<std::uint64_t, function_signature_s> lambda_signatures_;
  std::uint64_t next_lambda_id_;
  std::set<std::string> checked_files_;
  std::set<std::string> currently_checking_;
  std::vector<std::string> check_stack_;
  std::map<std::string, type_info_s> current_exports_;
  std::string current_file_;
  int loop_depth_{0};
};

} // namespace pkg::core::tcs
