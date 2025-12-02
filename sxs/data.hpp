#pragma once

#include <string>

namespace cmd::sxs::templates {

inline constexpr const char *EXAMPLE_KERNEL_CPP =
    R"(#include <sxs/kernel_api.hpp>
#include <iostream>

static const pkg::kernel::api_table_s *g_api = nullptr;

static slp::slp_object_c hello_world(pkg::kernel::context_t ctx,
                                      const slp::slp_object_c &args) {
  std::cout << "Hello from {PROJECT_NAME} kernel!" << std::endl;
  return slp::slp_object_c::create_string("Hello from {PROJECT_NAME}!");
}

static slp::slp_object_c add_numbers(pkg::kernel::context_t ctx,
                                      const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 3) {
    std::cerr << "add_numbers: ERROR - need 2 arguments" << std::endl;
    return slp::slp_object_c::create_int(0);
  }

  auto a = g_api->eval(ctx, list.at(1)).as_int();
  auto b = g_api->eval(ctx, list.at(2)).as_int();
  auto result = a + b;

  std::cout << "add_numbers: " << a << " + " << b << " = " << result << std::endl;
  return slp::slp_object_c::create_int(result);
}

static slp::slp_object_c greet_person(pkg::kernel::context_t ctx,
                                       const slp::slp_object_c &args) {
  auto list = args.as_list();
  if (list.size() < 2) {
    std::cerr << "greet_person: ERROR - need a name" << std::endl;
    return slp::slp_object_c::create_string("Hello, stranger!");
  }

  auto evaled = g_api->eval(ctx, list.at(1));
  if (evaled.type() != slp::slp_type_e::DQ_LIST) {
    std::cerr << "greet_person: ERROR - name must be a string" << std::endl;
    return slp::slp_object_c::create_string("Hello, stranger!");
  }

  auto name = evaled.as_string().to_string();
  std::cout << "greet_person: Hello, " << name << "!" << std::endl;

  return slp::slp_object_c::create_string(name);
}

extern "C" void kernel_init(pkg::kernel::registry_t registry,
                            const pkg::kernel::api_table_s *api) {
  g_api = api;
  api->register_function(registry, "hello_world", hello_world, 
                        slp::slp_type_e::DQ_LIST, 0);
  api->register_function(registry, "add_numbers", add_numbers, 
                        slp::slp_type_e::INTEGER, 0);
  api->register_function(registry, "greet_person", greet_person, 
                        slp::slp_type_e::DQ_LIST, 0);
}
)";

inline constexpr const char *KERNEL_SXS =
    R"(#(define-kernel {PROJECT_NAME} "libkernel_{PROJECT_NAME}.dylib" [
    (define-function hello_world () :str)
    (define-function add_numbers (a :int b :int) :int)
    (define-function greet_person (name :str) :str)
])
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
    #(import hello "modules/hello_world/hello_world.sxs")

    (debug "Starting {PROJECT_NAME}...")
    
    (hello/greet)
    (hello/say_message "Welcome to {PROJECT_NAME}!")
    
    (debug "Finished initialization")
]
)";

inline constexpr const char *HELLO_WORLD_MODULE = R"([
    (def greeting_count 0)

    (export greet (fn () :int [
        (debug "Hello from the hello_world module!")
        greeting_count
    ]))

    (export say_message (fn (msg :str) :str [
        (debug "Message:" msg)
        msg
    ]))
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
