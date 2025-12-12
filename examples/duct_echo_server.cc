#include "duct/duct.h"

#include <cstdlib>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "usage: " << argv[0] << " <address>\n";
    return 2;
  }

  std::string address = argv[1];
  auto lis = duct::listen(address);
  if (!lis.ok()) {
    std::cerr << "listen failed: " << static_cast<int>(lis.status().code()) << " "
              << lis.status().message() << "\n";
    return 1;
  }

  std::cerr << "listening on " << address << "\n";
  auto p = lis.value()->accept();
  if (!p.ok()) {
    std::cerr << "accept failed: " << static_cast<int>(p.status().code()) << " " << p.status().message()
              << "\n";
    return 1;
  }

  for (;;) {
    auto msg = p.value()->recv({});
    if (!msg.ok()) {
      std::cerr << "recv: " << static_cast<int>(msg.status().code()) << " " << msg.status().message() << "\n";
      break;
    }
    auto st = p.value()->send(msg.value(), {});
    if (!st.ok()) {
      std::cerr << "send: " << static_cast<int>(st.status().code()) << " " << st.status().message() << "\n";
      break;
    }
  }

  return 0;
}
