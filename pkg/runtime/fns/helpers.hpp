#pragma once
#include <optional>
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

#define SLP_TYPE_INT slp::parse(":int").take()
#define SLP_TYPE_REAL slp::parse(":real").take()
#define SLP_TYPE_STR slp::parse(":str").take()
#define SLP_TYPE_SOME slp::parse(":some").take()
#define SLP_TYPE_NONE slp::parse(":none").take()
#define SLP_TYPE_ERROR slp::parse(":error").take()
#define SLP_TYPE_SYMBOL slp::parse(":symbol").take()
#define SLP_TYPE_LIST_P slp::parse(":list-p").take()
#define SLP_TYPE_LIST_S slp::parse(":list-s").take()
#define SLP_TYPE_LIST_C slp::parse(":list-c").take()
#define SLP_TYPE_RUNE slp::parse(":rune").take()

inline std::optional<slp::slp_type_e>
type_symbol_to_enum(const std::string &sym) {
  if (sym == ":int")
    return slp::slp_type_e::INTEGER;
  if (sym == ":real")
    return slp::slp_type_e::REAL;
  if (sym == ":str")
    return slp::slp_type_e::DQ_LIST;
  if (sym == ":some")
    return slp::slp_type_e::SOME;
  if (sym == ":none")
    return slp::slp_type_e::NONE;
  if (sym == ":error")
    return slp::slp_type_e::ERROR;
  if (sym == ":symbol")
    return slp::slp_type_e::SYMBOL;
  if (sym == ":list-p")
    return slp::slp_type_e::PAREN_LIST;
  if (sym == ":list-s")
    return slp::slp_type_e::BRACKET_LIST;
  if (sym == ":list-c")
    return slp::slp_type_e::BRACE_LIST;
  if (sym == ":rune")
    return slp::slp_type_e::RUNE;
  return std::nullopt;
}
