#include "tui.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <string>

namespace cmd::sxs::tui {

using namespace ftxui;

class tui_c::impl {
public:
  impl() : input_content_(""), split_position_(20) {
    history_.push_back("SXS REPL - Enter commands below");
  }

  void run() {
    auto screen = ScreenInteractive::Fullscreen();

    auto input_component = Input(&input_content_, "Enter command...");

    auto input_with_handler = CatchEvent(input_component, [&](Event event) {
      if (event == Event::Return) {
        if (!input_content_.empty()) {
          history_.push_back(">>> " + input_content_);
          history_.push_back(input_content_);
          input_content_ = "";
          return true;
        }
      }
      if (event.is_character() && event.character() == "d" &&
          event.input().find("Ctrl") != std::string::npos) {
        screen.ExitLoopClosure()();
        return true;
      }
      return false;
    });

    auto output_renderer = Renderer([this] {
      Elements output_elements;
      for (const auto &line : history_) {
        output_elements.push_back(text(line));
      }
      return vbox(output_elements) | frame | flex;
    });

    auto input_renderer = Renderer(input_with_handler, [&] {
      return vbox({
                 separator(),
                 hbox({text(">>> "), input_with_handler->Render()}),
             }) |
             border;
    });

    int top_size = split_position_;
    auto container =
        ResizableSplitTop(output_renderer, input_renderer, &top_size);

    auto main_component = CatchEvent(container, [&](Event event) {
      if (event.is_character() && event.character() == "c" &&
          event.input().find("Ctrl") != std::string::npos) {
        screen.ExitLoopClosure()();
        return true;
      }
      return false;
    });

    screen.Loop(main_component);
  }

private:
  std::string input_content_;
  std::vector<std::string> history_;
  int split_position_;
};

tui_c::tui_c() : pimpl_(new impl()) {}

tui_c::~tui_c() { delete pimpl_; }

void tui_c::run() { pimpl_->run(); }

} // namespace cmd::sxs::tui
