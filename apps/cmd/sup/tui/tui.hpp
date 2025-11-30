#pragma once

namespace cmd::sxs::tui {

class tui_c {
public:
  tui_c();
  ~tui_c();

  void run();

private:
  class impl;
  impl *pimpl_;
};

} // namespace cmd::sxs::tui
