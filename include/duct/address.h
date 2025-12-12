#pragma once

#include <cstdint>
#include <string>

#include "duct/protocol.h"
#include "duct/status.h"

namespace duct {

struct TcpAddress {
  std::string host;
  std::uint16_t port = 0;
};

struct Address {
  Scheme scheme = Scheme::kUnknown;
  std::string scheme_text;  // original scheme token, for diagnostics
  std::string raw;     // original input

  // Parsed forms (only one is valid depending on scheme).
  TcpAddress tcp;
  std::string name;  // shm://<name>, pipe://<name>, ...

  static Result<Address> parse(std::string s);
};

}  // namespace duct
