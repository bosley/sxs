#pragma once

#include <string>

namespace cmd::sxs::templates {

inline constexpr const char *EXAMPLE_KERNEL_CPP =
    R"KERNEL(#include <sxs/kernel_api.hpp>
#include <iostream>
#include <cmath>
#include <vector>

static const pkg::kernel::api_table_s *g_api = nullptr;

static slp::slp_object_c hello_world(pkg::kernel::context_t ctx,
                                      const slp::slp_object_c &args) {
  std::cout << "Hello from {PROJECT_NAME} kernel!" << std::endl;
  return slp::slp_object_c::create_string("Hello from {PROJECT_NAME}!");
}

static slp::slp_object_c make_tuple(pkg::kernel::context_t ctx,
                                     const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 4) {
    std::cerr << "make_tuple: ERROR - need 3 arguments (int, str, real)" << std::endl;
    return slp::slp_object_c::create_none();
  }

  auto id = g_api->eval(ctx, list.at(1));
  auto name = g_api->eval(ctx, list.at(2));
  auto value = g_api->eval(ctx, list.at(3));

  if (id.type() != slp::slp_type_e::INTEGER) {
    std::cerr << "make_tuple: ERROR - first element must be int" << std::endl;
    return slp::slp_object_c::create_none();
  }
  if (name.type() != slp::slp_type_e::DQ_LIST) {
    std::cerr << "make_tuple: ERROR - second element must be str" << std::endl;
    return slp::slp_object_c::create_none();
  }
  if (value.type() != slp::slp_type_e::REAL) {
    std::cerr << "make_tuple: ERROR - third element must be real" << std::endl;
    return slp::slp_object_c::create_none();
  }

  std::vector<slp::slp_object_c> elements;
  elements.push_back(std::move(id));
  elements.push_back(std::move(name));
  elements.push_back(std::move(value));
  return slp::slp_object_c::create_brace_list(elements.data(), elements.size());
}

static slp::slp_object_c get_tuple_id(pkg::kernel::context_t ctx,
                                       const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 2) {
    std::cerr << "get_tuple_id: ERROR - need tuple argument" << std::endl;
    return slp::slp_object_c::create_int(0);
  }

  auto tuple = g_api->eval(ctx, list.at(1));
  if (tuple.type() != slp::slp_type_e::BRACE_LIST) {
    std::cerr << "get_tuple_id: ERROR - argument must be a tuple" << std::endl;
    return slp::slp_object_c::create_int(0);
  }

  auto tuple_list = tuple.as_list();
  if (tuple_list.size() < 3) {
    std::cerr << "get_tuple_id: ERROR - tuple must have 3 elements" << std::endl;
    return slp::slp_object_c::create_int(0);
  }

  return tuple_list.at(0);
}

static slp::slp_object_c tuple_summary(pkg::kernel::context_t ctx,
                                        const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 2) {
    std::cerr << "tuple_summary: ERROR - need tuple argument" << std::endl;
    return slp::slp_object_c::create_string("Invalid tuple");
  }

  auto tuple = g_api->eval(ctx, list.at(1));
  if (tuple.type() != slp::slp_type_e::BRACE_LIST) {
    std::cerr << "tuple_summary: ERROR - argument must be a tuple" << std::endl;
    return slp::slp_object_c::create_string("Invalid tuple");
  }

  auto tuple_list = tuple.as_list();
  if (tuple_list.size() < 3) {
    std::cerr << "tuple_summary: ERROR - tuple must have 3 elements" << std::endl;
    return slp::slp_object_c::create_string("Invalid tuple");
  }

  auto id = tuple_list.at(0).as_int();
  auto name = tuple_list.at(1).as_string().to_string();
  auto value = tuple_list.at(2).as_real();

  std::string summary = "Tuple[id=" + std::to_string(id) + 
                       ", name=" + name + 
                       ", value=" + std::to_string(value) + "]";

  return slp::slp_object_c::create_string(summary);
}

extern "C" void kernel_init(pkg::kernel::registry_t registry,
                            const pkg::kernel::api_table_s *api) {
  g_api = api;
  api->register_function(registry, "hello_world", hello_world, 
                        slp::slp_type_e::DQ_LIST, 0);
  api->register_function(registry, "make_tuple", make_tuple, 
                        slp::slp_type_e::BRACE_LIST, 0);
  api->register_function(registry, "get_tuple_id", get_tuple_id, 
                        slp::slp_type_e::INTEGER, 0);
  api->register_function(registry, "tuple_summary", tuple_summary, 
                        slp::slp_type_e::DQ_LIST, 0);
}
)KERNEL";

inline constexpr const char *KERNEL_SXS =
    R"([
    #(define-form tuple {:int :str :real})

    #(define-kernel {PROJECT_NAME} "libkernel_{PROJECT_NAME}.dylib" [
        (define-function hello_world () :str)
        (define-function make_tuple (id :int name :str value :real) :tuple)
        (define-function get_tuple_id (t :tuple) :int)
        (define-function tuple_summary (t :tuple) :str)
    ])
]
)";

inline constexpr const char *KERNEL_MAKEFILE = R"(CXX = clang++
SXS_HOME ?= $(HOME)/.sxs
CXXFLAGS = -std=c++20 -fPIC -I$(SXS_HOME)/include -I$(SXS_HOME)/include/sxs
LDFLAGS = -shared -L$(SXS_HOME)/lib -lpkg_slp

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    TARGET = libkernel_{PROJECT_NAME}.dylib
    LDFLAGS += -dynamiclib
else
    TARGET = libkernel_{PROJECT_NAME}.so
endif

all: $(TARGET)

$(TARGET): {PROJECT_NAME}.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ {PROJECT_NAME}.cpp

clean:
	rm -f $(TARGET)

.PHONY: all clean
)";

inline constexpr const char *INIT_SXS = R"([
    #(load "{PROJECT_NAME}")
    
    (debug "Starting {PROJECT_NAME}...")
    
    (debug "Creating a tuple with form :tuple defined in kernel...")
    (def my_tuple ({PROJECT_NAME}/make_tuple 42 "example" 3.14))
    
    (debug "Getting tuple ID...")
    (def tuple_id ({PROJECT_NAME}/get_tuple_id my_tuple))
    (debug "Tuple ID:" tuple_id)
    
    (debug "Getting tuple summary...")
    (def summary ({PROJECT_NAME}/tuple_summary my_tuple))
    (debug summary)
    
    (debug "Casting to :tuple type...")
    (def casted_tuple (cast :tuple my_tuple))
    (debug "Successfully cast to :tuple")
    
    (debug "Finished initialization")
]
)";

inline constexpr const char *GITIGNORE = R"(.sxs-cache/
*.dylib
*.so
*.o
)";

inline std::string replace_placeholder(std::string content,
                                       const std::string &placeholder,
                                       const std::string &value) {
  size_t pos = 0;
  while ((pos = content.find(placeholder, pos)) != std::string::npos) {
    content.replace(pos, placeholder.length(), value);
    pos += value.length();
  }
  return content;
}

} // namespace cmd::sxs::templates
