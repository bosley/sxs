#pragma once

#include "core/core.hpp"
#include "core/interpreter.hpp"
#include <map>
#include <set>
#include <string>

namespace pkg::core::imports {

/*

    This object will be created by the core and owned by the core.

    When an import occurs the datum interpreter will use the context interface
   (we will add new functions) to obtain a pointer to the imports_manager's
   add/remove virtual interface


*/

class import_context_if {
public:
  virtual ~import_context_if() = default;

  virtual bool is_import_allowed() = 0;

  virtual bool attempt_import(const std::string &symbol,
                              const std::string &file_path) = 0;

  virtual bool register_export(const std::string &name,
                               slp::slp_object_c &value) = 0;

  virtual void lock() = 0;
};

class imports_manager_c {
public:
  imports_manager_c(logger_t logger, std::vector<std::string> include_paths,
                    std::string working_directory,
                    std::map<std::string, std::unique_ptr<callable_context_if>>
                        *import_interpreters);
  ~imports_manager_c();

  import_context_if &get_import_context();

  void lock_imports();

  void set_parent_context(callable_context_if *context);

private:
  class import_guard_c {
  public:
    import_guard_c(imports_manager_c &manager,
                   const std::string &canonical_path);
    ~import_guard_c();
    import_guard_c(const import_guard_c &) = delete;
    import_guard_c &operator=(const import_guard_c &) = delete;

  private:
    imports_manager_c &manager_;
    std::string canonical_path_;
  };

  std::string resolve_file_path(const std::string &file_path);

  logger_t logger_;
  std::vector<std::string> include_paths_;
  std::string working_directory_;
  bool imports_locked_;
  std::set<std::string> imported_files_;
  std::set<std::string> currently_importing_;
  std::vector<std::string> import_stack_;
  std::map<std::string, slp::slp_object_c> current_exports_;
  callable_context_if *parent_context_;
  std::map<std::string, std::unique_ptr<callable_context_if>>
      *import_interpreters_;

  class import_context_c : public import_context_if {
  public:
    import_context_c(imports_manager_c &manager) : manager_(manager) {}
    ~import_context_c() override;

    bool is_import_allowed() override;
    bool attempt_import(const std::string &symbol,
                        const std::string &file_path) override;
    bool register_export(const std::string &name,
                         slp::slp_object_c &value) override;
    void lock() override;

  private:
    imports_manager_c &manager_;
  };

  std::unique_ptr<import_context_c> context_;
};
} // namespace pkg::core::imports