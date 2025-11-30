#pragma once

#include <string>

namespace cmd::sxs::templates {

inline constexpr const char *EXAMPLE_KERNEL_CPP = R"(#include <cstdio>
#include <sxs/kernel_api.h>
#include <sxs/slp/slp.hpp>

static const struct sxs_api_table_t *g_api = nullptr;

extern "C" void on_init(const struct sxs_api_table_t *api) {
  g_api = api;
  printf("{PROJECT_NAME} kernel initialized\n");
}

extern "C" void on_exit(const struct sxs_api_table_t *api) {
  printf("{PROJECT_NAME} kernel shutting down\n");
}

static sxs_object_t hello_world(sxs_context_t ctx, sxs_object_t args) {
  void *list = g_api->as_list(args);
  printf("Hello from {PROJECT_NAME} kernel!\n");
  return g_api->create_string("Hello from {PROJECT_NAME}!");
}

static sxs_object_t add_numbers(sxs_context_t ctx, sxs_object_t args) {
  void *list = g_api->as_list(args);
  if (g_api->list_size(list) < 3) {
    printf("add_numbers: ERROR - need 2 arguments\n");
    return g_api->create_int(0);
  }

  sxs_object_t arg1 = g_api->list_at(list, 1);
  sxs_object_t arg2 = g_api->list_at(list, 2);
  
  sxs_object_t evaled1 = g_api->eval(ctx, arg1);
  sxs_object_t evaled2 = g_api->eval(ctx, arg2);

  if (g_api->get_type(evaled1) != SXS_TYPE_INT || 
      g_api->get_type(evaled2) != SXS_TYPE_INT) {
    printf("add_numbers: ERROR - arguments must be integers\n");
    return g_api->create_int(0);
  }

  long long val1 = g_api->as_int(evaled1);
  long long val2 = g_api->as_int(evaled2);
  long long result = val1 + val2;

  printf("add_numbers: %lld + %lld = %lld\n", val1, val2, result);
  return g_api->create_int(result);
}

static sxs_object_t greet_person(sxs_context_t ctx, sxs_object_t args) {
  void *list = g_api->as_list(args);
  if (g_api->list_size(list) < 2) {
    printf("greet_person: ERROR - need a name\n");
    return g_api->create_string("Hello, stranger!");
  }

  sxs_object_t arg = g_api->list_at(list, 1);
  sxs_object_t evaled = g_api->eval(ctx, arg);

  if (g_api->get_type(evaled) != SXS_TYPE_STRING) {
    printf("greet_person: ERROR - name must be a string\n");
    return g_api->create_string("Hello, stranger!");
  }

  const char *name = g_api->as_string(evaled);
  printf("greet_person: Hello, %s!\n", name);

  return g_api->create_string(name);
}

extern "C" void kernel_init(sxs_registry_t registry,
                            const struct sxs_api_table_t *api) {
  g_api = api;
  api->register_function(registry, "hello_world", hello_world, SXS_TYPE_STRING, 0);
  api->register_function(registry, "add_numbers", add_numbers, SXS_TYPE_INT, 0);
  api->register_function(registry, "greet_person", greet_person, SXS_TYPE_STRING, 0);
}
)";

inline constexpr const char *KERNEL_SXS =
    R"(#(define-kernel {PROJECT_NAME} "libkernel_{PROJECT_NAME}.dylib" [
    (define-function hello_world () :str)
    (define-function add_numbers (a :int b :int) :int)
    (define-function greet_person (name :str) :str)

    (define-ctor on_init)
    (define-dtor on_exit)
])
)";

inline constexpr const char *KERNEL_MAKEFILE = R"(CXX = clang++
SXS_HOME ?= $(HOME)/.sxs
CXXFLAGS = -std=c++20 -fPIC -I$(SXS_HOME)/include
LDFLAGS = -shared

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
