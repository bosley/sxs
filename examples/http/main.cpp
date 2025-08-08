#include <fmt/core.h>
#include <httplib.h>

int main() {
  httplib::Client cli("http://httpbin.org");
  if (auto res = cli.Get("/get")) {
    fmt::print("Status: {}\nBody: {}\n", static_cast<int>(res->status),
               res->body);
  } else {
    fmt::print("Request failed: {}\n", httplib::to_string(res.error()));
  }
  return 0;
}
