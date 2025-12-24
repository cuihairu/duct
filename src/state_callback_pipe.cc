#include "state_callback_pipe.h"

#include <atomic>
#include <string>
#include <utility>

namespace duct {
namespace {

class StateCallbackPipe final : public Pipe {
 public:
  StateCallbackPipe(std::unique_ptr<Pipe> inner, ConnectionCallback cb)
      : inner_(std::move(inner)), cb_(std::move(cb)) {}

  ~StateCallbackPipe() override { close(); }

  Result<void> send(const Message& msg, const SendOptions& opt) override {
    if (!inner_) return Status::closed("pipe closed");
    auto st = inner_->send(msg, opt);
    if (!st.ok() && is_disconnect(st.status())) {
      emit_disconnected("send: " + st.status().message());
    }
    return st;
  }

  Result<Message> recv(const RecvOptions& opt) override {
    if (!inner_) return Status::closed("pipe closed");
    auto r = inner_->recv(opt);
    if (!r.ok() && is_disconnect(r.status())) {
      emit_disconnected("recv: " + r.status().message());
    }
    return r;
  }

  void close() override {
    bool expected = false;
    if (!closed_.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) return;

    if (cb_) cb_(ConnectionState::kClosed, "closed");
    if (inner_) inner_->close();
    inner_.reset();
  }

 private:
  bool is_disconnect(const Status& st) const {
    return st.code() == StatusCode::kClosed || st.code() == StatusCode::kIoError;
  }

  void emit_disconnected(const std::string& reason) {
    bool expected = false;
    if (!disconnected_.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) return;
    if (cb_) cb_(ConnectionState::kDisconnected, reason);
  }

  std::unique_ptr<Pipe> inner_;
  ConnectionCallback cb_;
  std::atomic<bool> closed_{false};
  std::atomic<bool> disconnected_{false};
};

}  // namespace

std::unique_ptr<Pipe> make_state_callback_pipe(std::unique_ptr<Pipe> inner, ConnectionCallback cb) {
  if (!cb) return inner;
  return std::make_unique<StateCallbackPipe>(std::move(inner), std::move(cb));
}

}  // namespace duct

