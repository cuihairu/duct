#include "duct/duct.h"

#include "duct/protocol.h"
#include "duct/wire.h"

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#if defined(_WIN32)
// TODO: Windows sockets/WSA startup + Named Pipe transport.
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace duct {
namespace {
// Framing constants are in duct::wire.

#if !defined(_WIN32)
// Framing helpers live in duct::wire.
#endif

class TcpPipe final : public Pipe {
 public:
  explicit TcpPipe(int fd) : fd_(fd) {}
  ~TcpPipe() override { close(); }

  Result<void> send(const Message& msg, const SendOptions&) override {
#if defined(_WIN32)
    (void)msg;
    return Status::not_supported("tcp not implemented on windows yet");
#else
    if (fd_ < 0) return Status::closed("pipe closed");
    return wire::write_frame(fd_, msg, /*flags=*/0);
#endif
  }

  Result<Message> recv(const RecvOptions&) override {
#if defined(_WIN32)
    return Status::not_supported("tcp not implemented on windows yet");
#else
    if (fd_ < 0) return Status::closed("pipe closed");
    return wire::read_frame(fd_);
#endif
  }

  void close() override {
#if defined(_WIN32)
    fd_ = -1;
#else
    if (fd_ >= 0) {
      ::close(fd_);
      fd_ = -1;
    }
#endif
  }

 private:
  int fd_ = -1;
};

class TcpListener final : public Listener {
 public:
  TcpListener(int fd, std::string host, std::uint16_t port) : fd_(fd), host_(std::move(host)), port_(port) {}
  ~TcpListener() override { close(); }

  Result<std::unique_ptr<Pipe>> accept() override {
#if defined(_WIN32)
    return Status::not_supported("tcp not implemented on windows yet");
#else
    if (fd_ < 0) return Status::closed("listener closed");
    int cfd = ::accept(fd_, nullptr, nullptr);
    if (cfd < 0) {
      return Status::io_error("accept() failed");
    }
    return std::unique_ptr<Pipe>(new TcpPipe(cfd));
#endif
  }

  Result<std::string> local_address() const override {
#if defined(_WIN32)
    return Status::not_supported("tcp not implemented on windows yet");
#else
    if (fd_ < 0) return Status::closed("listener closed");
    return std::string("tcp://") + (host_.empty() ? "127.0.0.1" : host_) + ":" + std::to_string(port_);
#endif
  }

  void close() override {
#if defined(_WIN32)
    fd_ = -1;
#else
    if (fd_ >= 0) {
      ::close(fd_);
      fd_ = -1;
    }
#endif
  }

 private:
  int fd_ = -1;
  std::string host_;
  std::uint16_t port_ = 0;
};

#if !defined(_WIN32)
static Result<int> connect_tcp(const std::string& host, std::uint16_t port) {
  struct addrinfo hints {};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  struct addrinfo* res = nullptr;
  std::string port_s = std::to_string(port);
  int rc = ::getaddrinfo(host.c_str(), port_s.c_str(), &hints, &res);
  if (rc != 0) {
    return Status::io_error("getaddrinfo() failed");
  }

  int fd = -1;
  for (auto* p = res; p != nullptr; p = p->ai_next) {
    fd = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (fd < 0) continue;

    int one = 1;
    (void)::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));

    if (::connect(fd, p->ai_addr, p->ai_addrlen) == 0) {
      break;
    }
    ::close(fd);
    fd = -1;
  }

  ::freeaddrinfo(res);
  if (fd < 0) {
    return Status::io_error("connect() failed");
  }
  return fd;
}

static Result<int> listen_tcp(const std::string& host, std::uint16_t port, int backlog) {
  struct addrinfo hints {};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  struct addrinfo* res = nullptr;
  std::string port_s = std::to_string(port);
  const char* node = host.empty() ? nullptr : host.c_str();
  int rc = ::getaddrinfo(node, port_s.c_str(), &hints, &res);
  if (rc != 0) {
    return Status::io_error("getaddrinfo() failed");
  }

  int fd = -1;
  for (auto* p = res; p != nullptr; p = p->ai_next) {
    fd = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (fd < 0) continue;

    int one = 1;
    (void)::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    if (::bind(fd, p->ai_addr, p->ai_addrlen) == 0 && ::listen(fd, backlog) == 0) {
      break;
    }
    ::close(fd);
    fd = -1;
  }

  ::freeaddrinfo(res);
  if (fd < 0) {
    return Status::io_error("bind/listen failed");
  }
  return fd;
}
#endif

}  // namespace

Result<std::unique_ptr<Listener>> tcp_listen(const TcpAddress& addr, const ListenOptions& opt) {
#if defined(_WIN32)
  (void)addr;
  (void)opt;
  return Status::not_supported("tcp not implemented on windows yet");
#else
  auto fd = listen_tcp(addr.host, addr.port, opt.backlog);
  if (!fd.ok()) return fd.status();

  // Determine effective port (supports binding to port 0).
  std::uint16_t effective_port = addr.port;
  if (effective_port == 0) {
    sockaddr_storage ss{};
    socklen_t slen = sizeof(ss);
    if (::getsockname(fd.value(), reinterpret_cast<sockaddr*>(&ss), &slen) == 0) {
      if (ss.ss_family == AF_INET) {
        auto* sin = reinterpret_cast<sockaddr_in*>(&ss);
        effective_port = ntohs(sin->sin_port);
      } else if (ss.ss_family == AF_INET6) {
        auto* sin6 = reinterpret_cast<sockaddr_in6*>(&ss);
        effective_port = ntohs(sin6->sin6_port);
      }
    }
  }

  return std::unique_ptr<Listener>(new TcpListener(fd.value(), addr.host, effective_port));
#endif
}

Result<std::unique_ptr<Pipe>> tcp_dial(const TcpAddress& addr, const DialOptions& opt) {
#if defined(_WIN32)
  (void)addr;
  (void)opt;
  return Status::not_supported("tcp not implemented on windows yet");
#else
  (void)opt;
  auto fd = connect_tcp(addr.host, addr.port);
  if (!fd.ok()) return fd.status();
  return std::unique_ptr<Pipe>(new TcpPipe(fd.value()));
#endif
}

}  // namespace duct
