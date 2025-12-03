#include "error_formatter.hpp"
#include "context.hpp"
#include <fmt/core.h>
#include <sstream>

namespace pkg::core {

std::string type_enum_to_string(slp::slp_type_e type) {
  switch (type) {
  case slp::slp_type_e::NONE:
    return "any";
  case slp::slp_type_e::SOME:
    return "some";
  case slp::slp_type_e::PAREN_LIST:
    return "list-p";
  case slp::slp_type_e::BRACE_LIST:
    return "list-c";
  case slp::slp_type_e::BRACKET_LIST:
    return "list-b";
  case slp::slp_type_e::DQ_LIST:
    return "str";
  case slp::slp_type_e::SYMBOL:
    return "symbol";
  case slp::slp_type_e::RUNE:
    return "rune";
  case slp::slp_type_e::INTEGER:
    return "int";
  case slp::slp_type_e::REAL:
    return "real";
  case slp::slp_type_e::ERROR:
    return "error";
  case slp::slp_type_e::DATUM:
    return "datum";
  case slp::slp_type_e::ABERRANT:
    return "lambda";
  default:
    return fmt::format("unknown({})", static_cast<int>(type));
  }
}

std::string type_to_string(const type_info_s &type) {
  std::string base = type_enum_to_string(type.base_type);

  if (!type.form_name.empty()) {
    base = fmt::format(":{}", type.form_name);
  }

  if (type.base_type == slp::slp_type_e::ABERRANT && type.lambda_id != 0) {
    base = fmt::format("lambda#{}", type.lambda_id);
  }

  if (type.is_variadic) {
    base += "..";
  }

  return base;
}

std::pair<size_t, size_t> byte_offset_to_line_col(const std::string &source,
                                                  size_t byte_offset) {
  size_t line = 1;
  size_t col = 1;

  for (size_t i = 0; i < byte_offset && i < source.size(); ++i) {
    if (source[i] == '\n') {
      line++;
      col = 1;
    } else {
      col++;
    }
  }

  return {line, col};
}

std::vector<std::string> get_context_lines(const std::string &source,
                                           size_t line_number,
                                           size_t context_size) {
  std::vector<std::string> lines;
  std::stringstream ss(source);
  std::string line;
  size_t current_line = 1;

  size_t start_line =
      line_number > context_size ? line_number - context_size : 1;
  size_t end_line = line_number + context_size;

  while (std::getline(ss, line)) {
    if (current_line >= start_line && current_line <= end_line) {
      lines.push_back(line);
    }
    current_line++;
    if (current_line > end_line) {
      break;
    }
  }

  return lines;
}

std::string colorize_error_output(const formatted_error_s &error) {
  std::stringstream output;

  output << "\033[1;31m" << error.title << "\033[0m\n";
  output << error.message << "\n\n";

  size_t context_start_line =
      error.error_line > error.context_lines.size() / 2
          ? error.error_line - error.context_lines.size() / 2
          : 1;

  for (size_t i = 0; i < error.context_lines.size(); ++i) {
    size_t line_num = context_start_line + i;
    bool is_error_line = (line_num == error.error_line);

    if (is_error_line) {
      output << fmt::format("\033[1;31m{:4} |\033[0m ", line_num);
      output << "\033[1;37m" << error.context_lines[i] << "\033[0m\n";

      output << "     | ";
      for (size_t j = 0; j < error.error_column - 1; ++j) {
        output << " ";
      }
      output << "\033[1;31m^\033[0m\n";
      output << "     | ";
      for (size_t j = 0; j < error.error_column - 1; ++j) {
        output << " ";
      }
      output << "\033[1;31m└─ error here\033[0m\n";
    } else {
      output << fmt::format("\033[90m{:4} |\033[0m ", line_num);
      output << "\033[90m" << error.context_lines[i] << "\033[0m\n";
    }
  }

  return output.str();
}

std::string format_type_error(const std::string &context_name,
                              const std::string &error_type,
                              const type_info_s &expected,
                              const type_info_s &actual,
                              const source_location_s &location) {
  formatted_error_s error;

  error.title = fmt::format("Type Error in {}", context_name);
  error.message = fmt::format(
      "{}: expected type '\033[1;32m{}\033[0m', but got '\033[1;33m{}\033[0m'",
      error_type, type_to_string(expected), type_to_string(actual));

  auto [line, col] =
      byte_offset_to_line_col(location.source_code, location.byte_offset);
  error.error_line = line;
  error.error_column = col;
  error.context_lines = get_context_lines(location.source_code, line, 3);

  std::stringstream output;
  output << "\n" << colorize_error_output(error);
  output << "\n\033[90mFile: " << location.file_path << ":" << line << ":"
         << col << "\033[0m\n";

  return output.str();
}

} // namespace pkg::core
