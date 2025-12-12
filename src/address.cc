#include "duct/address.h"

#include <charconv>
#include <string_view>

namespace duct {
namespace {

static bool parse_u16(std::string_view s, std::uint16_t* out) {
  unsigned v = 0;
  auto first = s.data();
  auto last = s.data() + s.size();
  auto r = std::from_chars(first, last, v);
  if (r.ec != std::errc() || r.ptr != last || v > 65535) {
    return false;
  }
  *out = static_cast<std::uint16_t>(v);
  return true;
}

}  // namespace

Result<Address> Address::parse(std::string s) {
  Address a;
  a.raw = s;

  // Accept either "tcp://host:port" or "host:port" for convenience.
  std::string_view sv{s};
  auto scheme_pos = sv.find("://");
  if (scheme_pos == std::string_view::npos) {
    a.scheme = Scheme::kTcp;
    a.scheme_text = "tcp";
  } else {
    a.scheme_text = std::string(sv.substr(0, scheme_pos));
    sv = sv.substr(scheme_pos + 3);
    if (a.scheme_text == "tcp") a.scheme = Scheme::kTcp;
    else if (a.scheme_text == "uds") a.scheme = Scheme::kUds;
    else if (a.scheme_text == "shm") a.scheme = Scheme::kShm;
    else if (a.scheme_text == "pipe") a.scheme = Scheme::kPipe;
    else a.scheme = Scheme::kUnknown;
  }

  if (a.scheme == Scheme::kTcp) {
    auto colon = sv.rfind(':');
    if (colon == std::string_view::npos) {
      return Status::invalid_argument("tcp address must be host:port");
    }
    a.tcp.host = std::string(sv.substr(0, colon));
    if (a.tcp.host.empty()) {
      a.tcp.host = "127.0.0.1";
    }
    std::uint16_t port = 0;
    if (!parse_u16(sv.substr(colon + 1), &port)) {
      return Status::invalid_argument("invalid tcp port");
    }
    a.tcp.port = port;
    return a;
  }

  if (a.scheme == Scheme::kShm || a.scheme == Scheme::kPipe) {
    if (sv.empty()) {
      return Status::invalid_argument(a.scheme_text + " address must be non-empty name");
    }
    a.name = std::string(sv);
    return a;
  }

  if (a.scheme == Scheme::kUds) {
    return Status::not_supported("uds scheme not implemented yet");
  }

  return Status::invalid_argument("unknown scheme: " + a.scheme_text);
}

}  // namespace duct
