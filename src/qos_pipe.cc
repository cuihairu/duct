#include "duct/qos_pipe.h"

#include <algorithm>

namespace duct {

QosPipe::QosPipe(std::unique_ptr<Pipe> underlying, const QosOptions& qos)
    : underlying_(std::move(underlying)), qos_(qos) {
  running_ = true;
  send_thread_ = std::thread(&QosPipe::send_worker, this);
}

QosPipe::~QosPipe() {
  close();
  if (send_thread_.joinable()) {
    send_thread_.join();
  }
}

bool QosPipe::can_send_message(const QueuedMessage& queued_msg) const {
  // Check TTL
  if (qos_.ttl.count() > 0) {
    auto now = std::chrono::steady_clock::now();
    auto age = now - queued_msg.timestamp;
    if (age > qos_.ttl) {
      return false;  // Message expired
    }
  }

  // Check queue limits
  if (send_bytes_.load() >= qos_.snd_hwm_bytes) {
    return false;
  }

  return true;
}

void QosPipe::send_worker() {
  while (running_) {
    std::unique_lock<std::mutex> lock(send_mutex_);

    // Wait for messages or shutdown
    send_cv_.wait(lock, [this] { return !send_queue_.empty() || !running_; });

    if (!running_) break;

    // Process expired messages (TTL)
    if (qos_.ttl.count() > 0) {
      auto now = std::chrono::steady_clock::now();

      send_queue_.erase(
        std::remove_if(send_queue_.begin(), send_queue_.end(),
          [&](const QueuedMessage& queued_msg) {
            auto age = now - queued_msg.timestamp;
            if (age > qos_.ttl) {
              send_bytes_ -= queued_msg.message.size();
              send_count_ -= 1;
              return true;  // Remove expired message
            }
            return false;
          }),
        send_queue_.end());
    }

    // Try to send the front message
    if (!send_queue_.empty()) {
      auto& queued_msg = send_queue_.front();

      // Check if we can send (respecting HWM limits)
      if (can_send_message(queued_msg)) {
        SendOptions opt;
        auto result = underlying_->send(queued_msg.message, opt);

        if (result.ok()) {
          // Message sent successfully
          send_bytes_ -= queued_msg.message.size();
          send_count_ -= 1;
          send_queue_.pop_front();
        } else {
          // Send failed - if it's a temporary issue, keep the message
          // If it's a permanent failure, we should close the pipe
          if (result.status().code() == StatusCode::kClosed ||
              result.status().code() == StatusCode::kIoError) {
            running_ = false;
            break;
          }
        }
      } else {
        // Hit HWM limits - check backpressure policy
        if (qos_.backpressure == BackpressurePolicy::kFailFast) {
          // Don't block, just let the next send() fail
          lock.unlock();
          std::this_thread::sleep_for(std::chrono::milliseconds(1));
        } else {
          // For kBlock, kDropNew, kDropOld, we handle them in send()
          lock.unlock();
          std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
      }
    }
  }
}

Result<void> QosPipe::send(const Message& msg, const SendOptions& opt) {
  if (!running_) {
    return Status::closed("pipe closed");
  }

  // Check message size against limits
  if (msg.size() > qos_.snd_hwm_bytes) {
    return Status::invalid_argument("message too large for queue limits");
  }

  std::unique_lock<std::mutex> lock(send_mutex_);

  // Handle backpressure policies
  if (send_bytes_.load() >= qos_.snd_hwm_bytes) {
    switch (qos_.backpressure) {
      case BackpressurePolicy::kFailFast:
        return Status::io_error("send queue at high water mark (fail fast)");

      case BackpressurePolicy::kDropNew:
        // Drop the new message
        return Status::Ok();

      case BackpressurePolicy::kDropOld:
        // Drop the oldest message if queue is not empty
        if (!send_queue_.empty()) {
          send_bytes_ -= send_queue_.front().message.size();
          send_count_ -= 1;
          send_queue_.pop_front();
        }
        break;

      case BackpressurePolicy::kBlock:
        // Wait for space in queue
        send_cv_.wait_for(lock, opt.timeout, [this] {
          return send_bytes_.load() < qos_.snd_hwm_bytes || !running_;
        });
        if (send_bytes_.load() >= qos_.snd_hwm_bytes) {
          return Status::timeout("send queue full (timeout)");
        }
        break;
    }
  }

  // Add message to send queue
  send_queue_.emplace_back(msg);
  send_bytes_ += msg.size();
  send_count_ += 1;

  lock.unlock();
  send_cv_.notify_one();

  return Status::Ok();
}

Result<Message> QosPipe::recv(const RecvOptions& opt) {
  return underlying_->recv(opt);
}

void QosPipe::close() {
  {
    std::lock_guard<std::mutex> lock(send_mutex_);
    running_ = false;
  }
  send_cv_.notify_all();
  underlying_->close();
}

}  // namespace duct