#pragma once

#include <memory>

namespace modules::http {

// This will associate multiple sk_<RANDOMHASH> keys to entity ids that 
// exist. the api key manager will utilize the entity permissions to restrict
// the entity approprriatly BEFORE runtime and DURING runtime (if need be)
class apikey_manager_c {
public:
  apikey_manager_c();
  ~apikey_manager_c() = default;
};


class http_module_c {
public:
  http_module_c();
  ~http_module_c() = default;


  std::unique_ptr<apikey_manager_c> get_apikey_manager();

};

} // namespace modules::http