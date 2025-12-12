#include "duct/duct.h"

#include <iostream>
#include <string>

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "usage: " << argv[0] << " <address> <message>\n";
    return 2;
  }

  std::string address = argv[1];
  std::string body = argv[2];

  auto p = duct::dial(address);
  if (!p.ok()) {
    std::cerr << "dial failed: " << static_cast<int>(p.status().code()) << " " << p.status().message() << "\n";
    return 1;
  }

  auto st = p.value()->send(duct::Message::from_string(body), {});
  if (!st.ok()) {
    std::cerr << "send failed: " << static_cast<int>(st.status().code()) << " " << st.status().message() << "\n";
    return 1;
  }

  auto r = p.value()->recv({});
  if (!r.ok()) {
    std::cerr << "recv failed: " << static_cast<int>(r.status().code()) << " " << r.status().message() << "\n";
    return 1;
  }

  std::string echoed(reinterpret_cast<const char*>(r.value().data()), r.value().size());
  std::cout << echoed << "\n";
  return 0;
}
