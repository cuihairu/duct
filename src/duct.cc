#include "duct/duct.h"

#include <memory>

namespace duct {

// Implemented in tcp_transport.cc.
Result<std::unique_ptr<Listener>> tcp_listen(const TcpAddress& addr, const ListenOptions& opt);
Result<std::unique_ptr<Pipe>> tcp_dial(const TcpAddress& addr, const DialOptions& opt);

// Implemented in shm_transport.cc.
Result<std::unique_ptr<Listener>> shm_listen(const std::string& name, const ListenOptions& opt);
Result<std::unique_ptr<Pipe>> shm_dial(const std::string& name, const DialOptions& opt);

Result<std::unique_ptr<Listener>> listen(const std::string& address, const ListenOptions& opt) {
  auto parsed = Address::parse(address);
  if (!parsed.ok()) {
    return parsed.status();
  }
  const Address& a = parsed.value();
  if (a.scheme == Scheme::kTcp) {
    return tcp_listen(a.tcp, opt);
  }
  if (a.scheme == Scheme::kShm) {
    return shm_listen(a.name, opt);
  }
  return Status::not_supported("listen scheme not supported yet: " + a.scheme_text);
}

Result<std::unique_ptr<Pipe>> dial(const std::string& address, const DialOptions& opt) {
  auto parsed = Address::parse(address);
  if (!parsed.ok()) {
    return parsed.status();
  }
  const Address& a = parsed.value();
  if (a.scheme == Scheme::kTcp) {
    return tcp_dial(a.tcp, opt);
  }
  if (a.scheme == Scheme::kShm) {
    return shm_dial(a.name, opt);
  }
  return Status::not_supported("dial scheme not supported yet: " + a.scheme_text);
}

}  // namespace duct
