#include "tui.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <string>
#include <vector>

using namespace ftxui;

namespace cmd::sxs::tui {

class tui_c::impl {
public:
  impl() : input_content_(""), scroll_y_(1.0f), screen_height_(100) {
    history_.push_back("SXS REPL - Interactive Console");
    history_.push_back(
        "Ctrl+D: Submit  |  Ctrl+C: Exit  |  Drag separator to resize");
    history_.push_back("");
    update_split_position();
  }

  void update_split_position() {
    split_position_ = static_cast<int>(screen_height_ * 0.7f);
  }

  void run() {
    auto screen = ScreenInteractive::Fullscreen();

    screen_height_ = screen.dimy();
    update_split_position();

    InputOption input_option;
    input_option.content = &input_content_;
    input_option.multiline = true;
    input_option.placeholder = "Type your command here (Ctrl+D to submit)...";
    input_option.on_enter = []() {};
    input_option.transform = [](InputState state) {
      state.element |= borderEmpty;
      if (state.is_placeholder) {
        state.element |= dim;
      }
      return state.element;
    };

    auto input_component = Input(input_option);

    Component input_with_handler = input_component;

    input_with_handler = CatchEvent(input_with_handler, [&](Event event) {
      if (event == Event::CtrlD) {
        if (!input_content_.empty()) {
          auto lines = split_lines(input_content_);
          for (const auto &line : lines) {
            history_.push_back(">>> " + line);
          }
          history_.push_back(input_content_);
          history_.push_back("");
          input_content_ = "";
          scroll_y_ = 1.0f;
        }
        return true;
      }
      if (event == Event::CtrlC) {
        screen.ExitLoopClosure()();
        return true;
      }
      return false;
    });

    auto output_content = Renderer([this] {
      Elements output_elements;
      for (const auto &line : history_) {
        output_elements.push_back(text(line));
      }
      return vbox(output_elements);
    });

    auto output_renderer = Renderer(output_content, [&] {
      return vbox({
                 text("Output:") | bold,
                 separator(),
                 output_content->Render() |
                     focusPositionRelative(0.0f, scroll_y_) | frame | flex,
             }) |
             border;
    });

    auto input_renderer = Renderer(input_with_handler, [&] {
      return vbox({
                 text("Input (Ctrl+D to submit):") | bold,
                 separator(),
                 input_with_handler->Render() | flex,
             }) |
             border | flex;
    });

    int top_size = split_position_;
    auto container =
        ResizableSplitTop(output_renderer, input_renderer, &top_size);

    input_component->TakeFocus();

    auto main_container = CatchEvent(container, [&](Event event) {
      if (event.is_cursor_position()) {
        return false;
      }
      if (event.screen_) {
        int new_height = event.screen_->dimy();
        if (new_height != screen_height_) {
          screen_height_ = new_height;
          if (top_size == split_position_) {
            update_split_position();
            top_size = split_position_;
          }
        }
      }
      return false;
    });

    screen.Loop(main_container);
  }

  std::vector<std::string> split_lines(const std::string &str) {
    std::vector<std::string> lines;
    std::string line;
    for (char c : str) {
      if (c == '\n') {
        lines.push_back(line);
        line.clear();
      } else {
        line += c;
      }
    }
    if (!line.empty()) {
      lines.push_back(line);
    }
    return lines;
  }

private:
  std::string input_content_;
  std::vector<std::string> history_;
  int split_position_;
  float scroll_y_;
  int screen_height_;
};

tui_c::tui_c() : pimpl_(new impl()) {}

tui_c::~tui_c() { delete pimpl_; }

void tui_c::run() { pimpl_->run(); }

} // namespace cmd::sxs::tui
