#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include "duct/address.h"
#include "duct/message.h"
#include "duct/status.h"

namespace duct {

enum class BackpressurePolicy {
  kBlock = 0,
  kDropNew,
  kDropOld,
  kFailFast,
};

enum class Reliability {
  kAtMostOnce = 0,
  kAtLeastOnce,
};

struct QosOptions {
  // Bytes are more stable than msg-count when payload sizes vary.
  std::size_t snd_hwm_bytes = 4 * 1024 * 1024;
  std::size_t rcv_hwm_bytes = 4 * 1024 * 1024;
  BackpressurePolicy backpressure = BackpressurePolicy::kBlock;

  // Per-message TTL; zero means disabled.
  std::chrono::milliseconds ttl{0};

  Reliability reliability = Reliability::kAtMostOnce;
};

struct SendOptions {
  // If non-zero, send will time out (where supported).
  std::chrono::milliseconds timeout{0};
  // Reserved for future: priority/channel_id.
};

struct RecvOptions {
  std::chrono::milliseconds timeout{0};
};

class Pipe {
 public:
  virtual ~Pipe() = default;
  virtual Result<void> send(const Message& msg, const SendOptions& opt) = 0;
  virtual Result<Message> recv(const RecvOptions& opt) = 0;
  virtual void close() = 0;
};

class Listener {
 public:
  virtual ~Listener() = default;
  virtual Result<std::unique_ptr<Pipe>> accept() = 0;
  // Returns the effective local address. Useful when listening on port 0 (ephemeral).
  // Default implementation returns kNotSupported.
  virtual Result<std::string> local_address() const {
    return Status::not_supported("local_address not supported");
  }
  virtual void close() = 0;
};

struct DialOptions {
  std::chrono::milliseconds timeout{0};
  QosOptions qos{};
};

struct ListenOptions {
  QosOptions qos{};
  int backlog = 128;
};

// Minimal entry points (pattern layer comes later).
Result<std::unique_ptr<Listener>> listen(const std::string& address, const ListenOptions& opt = {});
Result<std::unique_ptr<Pipe>> dial(const std::string& address, const DialOptions& opt = {});

}  // namespace duct
