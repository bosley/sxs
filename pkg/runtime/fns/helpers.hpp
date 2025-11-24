#pragma once
#include <slp/slp.hpp>

#define SLP_ERROR(msg)                                                         \
  ([]() {                                                                      \
    auto _r = slp::parse("@\"" msg "\"");                                      \
    return _r.take();                                                          \
  }())

#define SLP_BOOL(val)                                                          \
  ([](bool _v) {                                                               \
    auto _r = slp::parse(_v ? "true" : "false");                               \
    return _r.take();                                                          \
  }(val))

#define SLP_STRING(val)                                                        \
  ([](const std::string &_v) {                                                 \
    auto _r = slp::parse("\"" + _v + "\"");                                    \
    return _r.take();                                                          \
  }(val))

