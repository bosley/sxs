#include "slp/slp.hpp"
#include <cctype>
#include <cstring>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace slp {

struct parser_state_s {
    const std::string& source;
    size_t pos;
    std::vector<std::uint8_t> data_buffer;
    std::map<std::uint64_t, std::string> symbols;
    std::uint64_t next_symbol_id;
    
    parser_state_s(const std::string& src) : source(src), pos(0), next_symbol_id(1) {}
    
    bool at_end() const { return pos >= source.size(); }
    
    char current() const { return at_end() ? '\0' : source[pos]; }
    
    char peek(size_t offset = 1) const {
        size_t peek_pos = pos + offset;
        return peek_pos >= source.size() ? '\0' : source[peek_pos];
    }
    
    void advance() { if (!at_end()) pos++; }
    
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
            if (current() == '\n') advance();
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
    
    slp_unit_of_store_t* get_unit(size_t offset) {
        if (offset + sizeof(slp_unit_of_store_t) > data_buffer.size()) {
            return nullptr;
        }
        return reinterpret_cast<slp_unit_of_store_t*>(&data_buffer[offset]);
    }
};

struct parse_result_internal_s {
    std::optional<size_t> unit_offset;
    std::optional<slp_parse_error_s> error;
};

size_t allocate_unit(parser_state_s& state, slp_type_e type) {
    size_t offset = state.data_buffer.size();
    state.data_buffer.resize(offset + sizeof(slp_unit_of_store_t));
    slp_unit_of_store_t* unit = state.get_unit(offset);
    unit->header = static_cast<std::uint32_t>(type);
    unit->flags = 0;
    std::memset(&unit->data, 0, sizeof(data_u));
    return offset;
}

parse_result_internal_s parse_object(parser_state_s& state);

parse_result_internal_s parse_string(parser_state_s& state) {
    size_t start_pos = state.pos;
    state.advance();
    
    std::vector<size_t> char_offsets;
    
    while (!state.at_end() && state.current() != '"') {
        size_t rune_offset = allocate_unit(state, slp_type_e::RUNE);
        state.get_unit(rune_offset)->data.uint32 = static_cast<std::uint32_t>(state.current());
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
    if (!char_offsets.empty()) {
        state.get_unit(list_offset)->data.typed_range = state.get_unit(char_offsets[0]);
    }
    
    return parse_result_internal_s{list_offset, std::nullopt};
}

parse_result_internal_s parse_list(parser_state_s& state, char open, char close, slp_type_e type) {
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
    if (!element_offsets.empty()) {
        state.get_unit(list_offset)->data.typed_range = state.get_unit(element_offsets[0]);
    }
    
    return parse_result_internal_s{list_offset, std::nullopt};
}

parse_result_internal_s parse_atom(parser_state_s& state) {
    std::string atom;
    size_t start_pos = state.pos;
    
    while (!state.at_end() && 
           !std::isspace(state.current()) && 
           state.current() != ')' && 
           state.current() != ']' && 
           state.current() != '}' &&
           state.current() != ';') {
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
    
    if (atom[i] == '-' || atom[i] == '+') i++;
    
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

parse_result_internal_s handle_env(parser_state_s& state, size_t start_pos) {
    state.skip_whitespace_and_comments();
    
    if (state.at_end()) {
        slp_parse_error_s err;
        err.error_code = slp_parse_error_e::UNCLOSED_BRACKET_LIST;
        err.message = "env directive requires a name";
        err.byte_position = start_pos;
        return parse_result_internal_s{std::nullopt, err};
    }
    
    auto name_result = parse_object(state);
    if (name_result.error.has_value()) {
        return name_result;
    }
    
    std::vector<size_t> body_offsets;
    body_offsets.push_back(name_result.unit_offset.value());
    
    while (true) {
        state.skip_whitespace_and_comments();
        
        if (state.at_end()) {
            slp_parse_error_s err;
            err.error_code = slp_parse_error_e::UNCLOSED_BRACKET_LIST;
            err.message = "Unclosed env directive";
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
        
        body_offsets.push_back(elem_result.unit_offset.value());
    }
    
    size_t env_offset = allocate_unit(state, slp_type_e::ENVIRONMENT);
    if (!body_offsets.empty()) {
        state.get_unit(env_offset)->data.typed_range = state.get_unit(body_offsets[0]);
    }
    
    return parse_result_internal_s{env_offset, std::nullopt};
}

parse_result_internal_s handle_debug(parser_state_s& state, size_t start_pos) {
    state.skip_whitespace_and_comments();
    
    if (state.at_end()) {
        slp_parse_error_s err;
        err.error_code = slp_parse_error_e::UNCLOSED_BRACKET_LIST;
        err.message = "debug directive requires an object";
        err.byte_position = start_pos;
        return parse_result_internal_s{std::nullopt, err};
    }
    
    auto obj_result = parse_object(state);
    if (obj_result.error.has_value()) {
        return obj_result;
    }
    
    size_t obj_offset = obj_result.unit_offset.value();
    slp_unit_of_store_t* obj_unit = state.get_unit(obj_offset);
    slp_type_e obj_type = static_cast<slp_type_e>(obj_unit->header & 0xFF);
    
    std::cout << "[DEBUG] Object type: ";
    switch (obj_type) {
        case slp_type_e::SYMBOL: std::cout << "SYMBOL"; break;
        case slp_type_e::INTEGER: std::cout << "INTEGER"; break;
        case slp_type_e::REAL: std::cout << "REAL"; break;
        case slp_type_e::PAREN_LIST: std::cout << "PAREN_LIST"; break;
        case slp_type_e::BRACKET_LIST: std::cout << "BRACKET_LIST"; break;
        case slp_type_e::BRACE_LIST: std::cout << "BRACE_LIST"; break;
        default: std::cout << "UNKNOWN"; break;
    }
    std::cout << std::endl;
    
    state.skip_whitespace_and_comments();
    if (!state.at_end() && state.current() == ']') {
        state.advance();
    }
    
    return parse_result_internal_s{obj_offset, std::nullopt};
}

parse_result_internal_s handle_bracket_list(parser_state_s& state) {
    size_t start_pos = state.pos;
    state.advance();
    
    state.skip_whitespace_and_comments();
    
    if (state.at_end()) {
        slp_parse_error_s err;
        err.error_code = slp_parse_error_e::UNCLOSED_BRACKET_LIST;
        err.message = "Unclosed bracket list";
        err.byte_position = start_pos;
        return parse_result_internal_s{std::nullopt, err};
    }
    
    if (state.current() == ']') {
        state.advance();
        size_t list_offset = allocate_unit(state, slp_type_e::BRACKET_LIST);
        return parse_result_internal_s{list_offset, std::nullopt};
    }
    
    size_t directive_start = state.pos;
    std::string first_token;
    while (!state.at_end() && !std::isspace(state.current()) && state.current() != ']') {
        first_token += state.current();
        state.advance();
    }
    
    if (first_token == "env") {
        return handle_env(state, start_pos);
    } else if (first_token == "debug") {
        return handle_debug(state, start_pos);
    }
    
    state.pos = directive_start;
    
    std::vector<size_t> element_offsets;
    
    while (true) {
        state.skip_whitespace_and_comments();
        
        if (state.at_end()) {
            slp_parse_error_s err;
            err.error_code = slp_parse_error_e::UNCLOSED_BRACKET_LIST;
            err.message = "Unclosed bracket list";
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
    
    size_t list_offset = allocate_unit(state, slp_type_e::BRACKET_LIST);
    if (!element_offsets.empty()) {
        state.get_unit(list_offset)->data.typed_range = state.get_unit(element_offsets[0]);
    }
    
    return parse_result_internal_s{list_offset, std::nullopt};
}

parse_result_internal_s parse_object(parser_state_s& state) {
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
        
        size_t some_offset = allocate_unit(state, slp_type_e::SOME);
        state.get_unit(some_offset)->data.data_ptr = reinterpret_cast<data_u*>(state.get_unit(inner_result.unit_offset.value()));
        
        return parse_result_internal_s{some_offset, std::nullopt};
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

slp_object_c::slp_object_c() : view_(nullptr), root_offset_(0) {}

slp_object_c::~slp_object_c() {
    view_ = nullptr;
}

slp_object_c::slp_object_c(slp_object_c&& other) noexcept
    : view_(nullptr), data_(std::move(other.data_)), root_offset_(other.root_offset_), symbols_(std::move(other.symbols_)) {
    if (root_offset_ < data_.size()) {
        view_ = reinterpret_cast<slp_unit_of_store_t*>(&data_[root_offset_]);
    }
    other.view_ = nullptr;
    other.root_offset_ = 0;
}

slp_object_c& slp_object_c::operator=(slp_object_c&& other) noexcept {
    if (this != &other) {
        data_ = std::move(other.data_);
        root_offset_ = other.root_offset_;
        symbols_ = std::move(other.symbols_);
        if (root_offset_ < data_.size()) {
            view_ = reinterpret_cast<slp_unit_of_store_t*>(&data_[root_offset_]);
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

const char* slp_object_c::as_symbol() const {
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

bool slp_object_c::has_data() const {
    return view_ != nullptr && !data_.empty();
}

slp_parse_result_c::slp_parse_result_c()
    : error_(std::nullopt), object_(std::nullopt) {}

slp_parse_result_c::~slp_parse_result_c() {}

slp_parse_result_c parse(const std::string& source) {
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
        obj.view_ = reinterpret_cast<slp_unit_of_store_t*>(&obj.data_[obj.root_offset_]);
    }
    
    parse_result.object_ = std::move(obj);
    
    return parse_result;
}

} // namespace slp