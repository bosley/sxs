#include "slp/slp.hpp"
#include <cctype>
#include <cstring>
#include <optional>
#include <string>
#include <vector>

namespace slp {

struct parser_state_s {
  const std::string &source;
  size_t pos;
  slp_buffer_c data_buffer;
  std::map<std::uint64_t, std::string> symbols;
  std::uint64_t next_symbol_id;

  parser_state_s(const std::string &src)
      : source(src), pos(0), next_symbol_id(1) {}

  bool at_end() const { return pos >= source.size(); }

  char current() const { return at_end() ? '\0' : source[pos]; }

  char peek(size_t offset = 1) const {
    size_t peek_pos = pos + offset;
    return peek_pos >= source.size() ? '\0' : source[peek_pos];
  }

  void advance() {
    if (!at_end())
      pos++;
  }

  void skip_whitespace() {
    while (!at_end() && std::isspace(current())) {
      advance();
    }
  }

  void skip_comment() {
    if (current() == ';') {
      while (!at_end() && current() != '\n') {
        advance();
      }
      if (current() == '\n')
        advance();
    }
  }

  void skip_whitespace_and_comments() {
    while (true) {
      skip_whitespace();
      if (current() == ';') {
        skip_comment();
      } else {
        break;
      }
    }
  }

  slp_unit_of_store_t *get_unit(size_t offset) {
    if (offset + sizeof(slp_unit_of_store_t) > data_buffer.size()) {
      return nullptr;
    }
    return reinterpret_cast<slp_unit_of_store_t *>(&data_buffer[offset]);
  }
};

struct parse_result_internal_s {
  std::optional<size_t> unit_offset;
  std::optional<slp_parse_error_s> error;
};

size_t allocate_unit(parser_state_s &state, slp_type_e type) {
  size_t offset = state.data_buffer.size();
  state.data_buffer.resize(offset + sizeof(slp_unit_of_store_t));
  slp_unit_of_store_t *unit = state.get_unit(offset);
  unit->header = static_cast<std::uint32_t>(type);
  unit->flags = 0;
  std::memset(&unit->data, 0, sizeof(data_u));
  return offset;
}

parse_result_internal_s parse_object(parser_state_s &state);

parse_result_internal_s parse_string(parser_state_s &state) {
  size_t start_pos = state.pos;
  state.advance();

  std::vector<size_t> char_offsets;

  while (!state.at_end() && state.current() != '"') {
    char c = state.current();
    if (c == '\\' && state.peek() != '\0') {
      state.advance();
      c = state.current();
    }
    size_t rune_offset = allocate_unit(state, slp_type_e::RUNE);
    state.get_unit(rune_offset)->data.uint32 =
        static_cast<std::uint32_t>(c);
    char_offsets.push_back(rune_offset);
    state.advance();
  }

  if (state.at_end()) {
    slp_parse_error_s err;
    err.error_code = slp_parse_error_e::UNCLOSED_DQ_LIST;
    err.message = "Unclosed string";
    err.byte_position = start_pos;
    return parse_result_internal_s{std::nullopt, err};
  }

  state.advance();

  size_t list_offset = allocate_unit(state, slp_type_e::DQ_LIST);
  size_t offsets_array_pos = 0;

  if (!char_offsets.empty()) {
    offsets_array_pos = state.data_buffer.size();
    for (size_t offset : char_offsets) {
      state.data_buffer.insert(state.data_buffer.size(),
                               reinterpret_cast<const std::uint8_t *>(&offset),
                               sizeof(size_t));
    }
  }

  slp_unit_of_store_t *list_unit = state.get_unit(list_offset);
  list_unit->flags = static_cast<std::uint32_t>(char_offsets.size());
  if (!char_offsets.empty()) {
    list_unit->data.uint64 = static_cast<std::uint64_t>(offsets_array_pos);
  }

  return parse_result_internal_s{list_offset, std::nullopt};
}

parse_result_internal_s parse_list(parser_state_s &state, char open, char close,
                                   slp_type_e type) {
  size_t start_pos = state.pos;
  state.advance();

  std::vector<size_t> element_offsets;

  while (true) {
    state.skip_whitespace_and_comments();

    if (state.at_end()) {
      slp_parse_error_s err;
      if (type == slp_type_e::PAREN_LIST) {
        err.error_code = slp_parse_error_e::UNCLOSED_PAREN_LIST;
        err.message = "Unclosed paren list";
      } else if (type == slp_type_e::BRACKET_LIST) {
        err.error_code = slp_parse_error_e::UNCLOSED_BRACKET_LIST;
        err.message = "Unclosed bracket list";
      } else {
        err.error_code = slp_parse_error_e::UNCLOSED_BRACE_LIST;
        err.message = "Unclosed brace list";
      }
      err.byte_position = start_pos;
      return parse_result_internal_s{std::nullopt, err};
    }

    if (state.current() == close) {
      state.advance();
      break;
    }

    auto elem_result = parse_object(state);
    if (elem_result.error.has_value()) {
      return elem_result;
    }

    element_offsets.push_back(elem_result.unit_offset.value());
  }

  size_t list_offset = allocate_unit(state, type);
  size_t offsets_array_pos = 0;

  if (!element_offsets.empty()) {
    offsets_array_pos = state.data_buffer.size();
    for (size_t offset : element_offsets) {
      state.data_buffer.insert(state.data_buffer.size(),
                               reinterpret_cast<const std::uint8_t *>(&offset),
                               sizeof(size_t));
    }
  }

  slp_unit_of_store_t *list_unit = state.get_unit(list_offset);
  list_unit->flags = static_cast<std::uint32_t>(element_offsets.size());
  if (!element_offsets.empty()) {
    list_unit->data.uint64 = static_cast<std::uint64_t>(offsets_array_pos);
  }

  return parse_result_internal_s{list_offset, std::nullopt};
}

parse_result_internal_s parse_atom(parser_state_s &state) {
  std::string atom;
  size_t start_pos = state.pos;

  while (!state.at_end() && !std::isspace(state.current()) &&
         state.current() != ')' && state.current() != ']' &&
         state.current() != '}' && state.current() != ';') {
    atom += state.current();
    state.advance();
  }

  if (atom.empty()) {
    return parse_result_internal_s{std::nullopt, std::nullopt};
  }

  bool is_number = true;
  bool has_decimal = false;
  bool has_exponent = false;
  size_t i = 0;

  if (atom[i] == '-' || atom[i] == '+')
    i++;

  if (i >= atom.size() || (!std::isdigit(atom[i]) && atom[i] != '.')) {
    is_number = false;
  }

  while (is_number && i < atom.size()) {
    if (std::isdigit(atom[i])) {
      i++;
    } else if (atom[i] == '.' && !has_decimal && !has_exponent) {
      has_decimal = true;
      i++;
    } else if ((atom[i] == 'e' || atom[i] == 'E') && !has_exponent) {
      has_exponent = true;
      has_decimal = true;
      i++;
      if (i < atom.size() && (atom[i] == '+' || atom[i] == '-')) {
        i++;
      }
    } else {
      is_number = false;
      break;
    }
  }

  if (is_number && i == atom.size()) {
    if (has_decimal) {
      size_t unit_offset = allocate_unit(state, slp_type_e::REAL);
      state.get_unit(unit_offset)->data.float64 = std::stod(atom);
      return parse_result_internal_s{unit_offset, std::nullopt};
    } else {
      size_t unit_offset = allocate_unit(state, slp_type_e::INTEGER);
      state.get_unit(unit_offset)->data.int64 = std::stoll(atom);
      return parse_result_internal_s{unit_offset, std::nullopt};
    }
  }

  size_t unit_offset = allocate_unit(state, slp_type_e::SYMBOL);
  std::uint64_t symbol_id = state.next_symbol_id++;
  state.symbols[symbol_id] = atom;
  state.get_unit(unit_offset)->data.uint64 = symbol_id;

  return parse_result_internal_s{unit_offset, std::nullopt};
}

parse_result_internal_s handle_bracket_list(parser_state_s &state) {
  size_t start_pos = state.pos;
  state.advance();

  std::vector<size_t> element_offsets;

  while (true) {
    state.skip_whitespace_and_comments();

    if (state.at_end()) {
      slp_parse_error_s err;
      err.error_code = slp_parse_error_e::UNCLOSED_BRACKET_LIST;
      err.message = "Unclosed environment";
      err.byte_position = start_pos;
      return parse_result_internal_s{std::nullopt, err};
    }

    if (state.current() == ']') {
      state.advance();
      break;
    }

    auto elem_result = parse_object(state);
    if (elem_result.error.has_value()) {
      return elem_result;
    }

    element_offsets.push_back(elem_result.unit_offset.value());
  }

  size_t env_offset = allocate_unit(state, slp_type_e::BRACKET_LIST);
  size_t offsets_array_pos = 0;

  if (!element_offsets.empty()) {
    offsets_array_pos = state.data_buffer.size();
    for (size_t offset : element_offsets) {
      state.data_buffer.insert(state.data_buffer.size(),
                               reinterpret_cast<const std::uint8_t *>(&offset),
                               sizeof(size_t));
    }
  }

  slp_unit_of_store_t *env_unit = state.get_unit(env_offset);
  env_unit->flags = static_cast<std::uint32_t>(element_offsets.size());
  if (!element_offsets.empty()) {
    env_unit->data.uint64 = static_cast<std::uint64_t>(offsets_array_pos);
  }

  return parse_result_internal_s{env_offset, std::nullopt};
}

parse_result_internal_s parse_object(parser_state_s &state) {
  state.skip_whitespace_and_comments();

  if (state.at_end()) {
    return parse_result_internal_s{std::nullopt, std::nullopt};
  }

  char c = state.current();

  if (c == '\'') {
    state.advance();
    auto inner_result = parse_object(state);
    if (inner_result.error.has_value()) {
      return inner_result;
    }

    if (!inner_result.unit_offset.has_value()) {
      slp_parse_error_s err;
      err.error_code = slp_parse_error_e::ERROR_OPERATOR_REQUIRES_OBJECT;
      err.message = "Quote operator requires an object";
      err.byte_position = state.pos;
      return parse_result_internal_s{std::nullopt, err};
    }

    size_t some_offset = allocate_unit(state, slp_type_e::SOME);
    state.get_unit(some_offset)->data.data_ptr = reinterpret_cast<data_u *>(
        state.get_unit(inner_result.unit_offset.value()));

    return parse_result_internal_s{some_offset, std::nullopt};
  }

  if (c == '@') {
    state.advance();
    auto inner_result = parse_object(state);
    if (inner_result.error.has_value()) {
      return inner_result;
    }

    if (!inner_result.unit_offset.has_value()) {
      slp_parse_error_s err;
      err.error_code = slp_parse_error_e::ERROR_OPERATOR_REQUIRES_OBJECT;
      err.message = "Error operator requires an object";
      err.byte_position = state.pos;
      return parse_result_internal_s{std::nullopt, err};
    }

    size_t error_offset = allocate_unit(state, slp_type_e::ERROR);
    state.get_unit(error_offset)->data.data_ptr = reinterpret_cast<data_u *>(
        state.get_unit(inner_result.unit_offset.value()));

    return parse_result_internal_s{error_offset, std::nullopt};
  }

  if (c == '(') {
    return parse_list(state, '(', ')', slp_type_e::PAREN_LIST);
  }

  if (c == '[') {
    return handle_bracket_list(state);
  }

  if (c == '{') {
    return parse_list(state, '{', '}', slp_type_e::BRACE_LIST);
  }

  if (c == '"') {
    return parse_string(state);
  }

  return parse_atom(state);
}

slp_object_c::list_c::list_c() : parent_(nullptr), is_valid_(false) {}

slp_object_c::list_c::list_c(const slp_object_c *parent)
    : parent_(parent), is_valid_(false) {
  if (!parent_ || !parent_->view_) {
    return;
  }
  slp_type_e t = parent_->type();
  is_valid_ = (t == slp_type_e::PAREN_LIST || t == slp_type_e::BRACE_LIST ||
               t == slp_type_e::BRACKET_LIST);
}

size_t slp_object_c::list_c::size() const {
  if (!is_valid_ || !parent_ || !parent_->view_) {
    return 0;
  }

  return parent_->view_->flags;
}

bool slp_object_c::list_c::empty() const { return size() == 0; }

slp_object_c slp_object_c::list_c::at(size_t index) const {
  slp_object_c result;

  if (!is_valid_ || !parent_ || !parent_->view_) {
    return result;
  }

  if (index >= size()) {
    return result;
  }

  size_t offsets_array_pos = static_cast<size_t>(parent_->view_->data.uint64);
  const size_t *offsets_array =
      reinterpret_cast<const size_t *>(&parent_->data_[offsets_array_pos]);

  size_t target_offset = offsets_array[index];

  if (target_offset + sizeof(slp_unit_of_store_t) > parent_->data_.size()) {
    return result;
  }

  result.data_ = parent_->data_;
  result.symbols_ = parent_->symbols_;
  result.root_offset_ = target_offset;
  result.view_ =
      reinterpret_cast<slp_unit_of_store_t *>(&result.data_[target_offset]);

  return result;
}

slp_object_c::string_c::string_c() : parent_(nullptr), is_valid_(false) {}

slp_object_c::string_c::string_c(const slp_object_c *parent)
    : parent_(parent), is_valid_(false) {
  if (!parent_ || !parent_->view_) {
    return;
  }
  is_valid_ = (parent_->type() == slp_type_e::DQ_LIST);
}

size_t slp_object_c::string_c::size() const {
  if (!is_valid_ || !parent_ || !parent_->view_) {
    return 0;
  }

  return parent_->view_->flags;
}

bool slp_object_c::string_c::empty() const { return size() == 0; }

char slp_object_c::string_c::at(size_t index) const {
  if (!is_valid_ || !parent_ || !parent_->view_) {
    return '\0';
  }

  if (index >= size()) {
    return '\0';
  }

  size_t offsets_array_pos = static_cast<size_t>(parent_->view_->data.uint64);
  const size_t *offsets_array =
      reinterpret_cast<const size_t *>(&parent_->data_[offsets_array_pos]);

  size_t target_offset = offsets_array[index];

  if (target_offset + sizeof(slp_unit_of_store_t) > parent_->data_.size()) {
    return '\0';
  }

  slp_unit_of_store_t *rune_unit = reinterpret_cast<slp_unit_of_store_t *>(
      const_cast<std::uint8_t *>(&parent_->data_[target_offset]));

  return static_cast<char>(rune_unit->data.uint32);
}

std::string slp_object_c::string_c::to_string() const {
  std::string result;
  size_t len = size();
  result.reserve(len);

  for (size_t i = 0; i < len; ++i) {
    result += at(i);
  }

  return result;
}

slp_object_c::slp_object_c() : view_(nullptr), root_offset_(0) {}

slp_object_c::~slp_object_c() { view_ = nullptr; }

slp_object_c::slp_object_c(slp_object_c &&other) noexcept
    : view_(nullptr), data_(std::move(other.data_)),
      root_offset_(other.root_offset_), symbols_(std::move(other.symbols_)) {
  if (root_offset_ < data_.size()) {
    view_ = reinterpret_cast<slp_unit_of_store_t *>(&data_[root_offset_]);
  }
  other.view_ = nullptr;
  other.root_offset_ = 0;
}

slp_object_c &slp_object_c::operator=(slp_object_c &&other) noexcept {
  if (this != &other) {
    data_ = std::move(other.data_);
    root_offset_ = other.root_offset_;
    symbols_ = std::move(other.symbols_);
    if (root_offset_ < data_.size()) {
      view_ = reinterpret_cast<slp_unit_of_store_t *>(&data_[root_offset_]);
    } else {
      view_ = nullptr;
    }
    other.view_ = nullptr;
    other.root_offset_ = 0;
  }
  return *this;
}

slp_type_e slp_object_c::type() const {
  if (!view_) {
    return slp_type_e::NONE;
  }
  return static_cast<slp_type_e>(view_->header & 0xFF);
}

std::int64_t slp_object_c::as_int() const {
  if (!view_ || type() != slp_type_e::INTEGER) {
    return 0;
  }
  return view_->data.int64;
}

double slp_object_c::as_real() const {
  if (!view_ || type() != slp_type_e::REAL) {
    return 0.0;
  }
  return view_->data.float64;
}

const char *slp_object_c::as_symbol() const {
  if (!view_ || type() != slp_type_e::SYMBOL) {
    return "";
  }
  std::uint64_t symbol_id = view_->data.uint64;
  auto it = symbols_.find(symbol_id);
  if (it == symbols_.end()) {
    return "";
  }
  return it->second.c_str();
}

slp_object_c::list_c slp_object_c::as_list() const { return list_c(this); }

slp_object_c::string_c slp_object_c::as_string() const {
  return string_c(this);
}

bool slp_object_c::has_data() const {
  return view_ != nullptr && !data_.empty();
}

const slp_buffer_c &slp_object_c::get_data() const { return data_; }

const std::map<std::uint64_t, std::string> &slp_object_c::get_symbols() const {
  return symbols_;
}

size_t slp_object_c::get_root_offset() const { return root_offset_; }

slp_object_c
slp_object_c::from_data(const slp_buffer_c &data,
                        const std::map<std::uint64_t, std::string> &symbols,
                        size_t root_offset) {
  slp_object_c obj;
  obj.data_ = data;
  obj.symbols_ = symbols;
  obj.root_offset_ = root_offset;

  if (obj.root_offset_ < obj.data_.size()) {
    obj.view_ = reinterpret_cast<slp_unit_of_store_t *>(
        const_cast<std::uint8_t *>(&obj.data_[obj.root_offset_]));
  }

  return obj;
}

slp_parse_result_c::slp_parse_result_c()
    : error_(std::nullopt), object_(std::nullopt) {}

slp_parse_result_c::~slp_parse_result_c() {}

slp_parse_result_c parse(const std::string &source) {
  parser_state_s state(source);

  auto result = parse_object(state);

  slp_parse_result_c parse_result;

  if (result.error.has_value()) {
    parse_result.error_ = result.error.value();
    return parse_result;
  }

  if (!result.unit_offset.has_value()) {
    slp_parse_error_s err;
    err.error_code = slp_parse_error_e::MALFORMED_NUMERIC_LITERAL;
    err.message = "No object found in source";
    err.byte_position = 0;
    parse_result.error_ = err;
    return parse_result;
  }

  slp_object_c obj;
  obj.data_ = std::move(state.data_buffer);
  obj.root_offset_ = result.unit_offset.value();
  obj.symbols_ = std::move(state.symbols);

  if (obj.root_offset_ < obj.data_.size()) {
    obj.view_ =
        reinterpret_cast<slp_unit_of_store_t *>(&obj.data_[obj.root_offset_]);
  }

  parse_result.object_ = std::move(obj);

  return parse_result;
}

} // namespace slp