#pragma once

#include <memory>
#include <utility>

#include "duct/duct.h"

namespace duct::raii {

// ==============================================================================
// RAII 包装器
// ==============================================================================

/**
 * @brief Pipe 的 RAII 包装器，确保在析构时自动关闭
 */
class ScopedPipe {
 public:
  ScopedPipe() = default;
  explicit ScopedPipe(std::unique_ptr<Pipe> pipe) : pipe_(std::move(pipe)) {}

  ~ScopedPipe() { close(); }

  // 禁止拷贝
  ScopedPipe(const ScopedPipe&) = delete;
  ScopedPipe& operator=(const ScopedPipe&) = delete;

  // 支持移动
  ScopedPipe(ScopedPipe&& other) noexcept : pipe_(std::exchange(other.pipe_, nullptr)) {}
  ScopedPipe& operator=(ScopedPipe&& other) noexcept {
    if (this != &other) {
      close();
      pipe_ = std::exchange(other.pipe_, nullptr);
    }
    return *this;
  }

  // 访问底层 Pipe
  Pipe* get() const { return pipe_.get(); }
  Pipe* operator->() const { return pipe_.get(); }
  Pipe& operator*() const { return *pipe_; }

  // 释放所有权
  [[nodiscard]] std::unique_ptr<Pipe> release() {
    return std::move(pipe_);
  }

  // 手动关闭
  void close() {
    if (pipe_) {
      pipe_->close();
      pipe_.reset();
    }
  }

  // 检查是否有效
  explicit operator bool() const { return pipe_ != nullptr; }

  // 转发方法
  Result<void> send(const Message& msg, const SendOptions& opt = {}) {
    return pipe_->send(msg, opt);
  }

  Result<Message> recv(const RecvOptions& opt = {}) {
    return pipe_->recv(opt);
  }

 private:
  std::unique_ptr<Pipe> pipe_;
};

/**
 * @brief Listener 的 RAII 包装器，确保在析构时自动关闭
 */
class ScopedListener {
 public:
  ScopedListener() = default;
  explicit ScopedListener(std::unique_ptr<Listener> listener) : listener_(std::move(listener)) {}

  ~ScopedListener() { close(); }

  // 禁止拷贝
  ScopedListener(const ScopedListener&) = delete;
  ScopedListener& operator=(const ScopedListener&) = delete;

  // 支持移动
  ScopedListener(ScopedListener&& other) noexcept : listener_(std::exchange(other.listener_, nullptr)) {}
  ScopedListener& operator=(ScopedListener&& other) noexcept {
    if (this != &other) {
      close();
      listener_ = std::exchange(other.listener_, nullptr);
    }
    return *this;
  }

  // 访问底层 Listener
  Listener* get() const { return listener_.get(); }
  Listener* operator->() const { return listener_.get(); }
  Listener& operator*() const { return *listener_; }

  // 释放所有权
  [[nodiscard]] std::unique_ptr<Listener> release() {
    return std::move(listener_);
  }

  // 手动关闭
  void close() {
    if (listener_) {
      listener_->close();
      listener_.reset();
    }
  }

  // 检查是否有效
  explicit operator bool() const { return listener_ != nullptr; }

  // 转发方法
  Result<ScopedPipe> accept() {
    auto result = listener_->accept();
    if (result.ok()) {
      return ScopedPipe(std::move(result).value());
    }
    return result.status();
  }

  Result<std::string> local_address() const {
    return listener_->local_address();
  }

 private:
  std::unique_ptr<Listener> listener_;
};

// ==============================================================================
// 便捷工厂函数
// ==============================================================================

/**
 * @brief RAII 风格的连接
 */
inline Result<ScopedPipe> connect(const std::string& address, const DialOptions& opt = {}) {
  auto result = dial(address, opt);
  if (result.ok()) {
    return ScopedPipe(std::move(result).value());
  }
  return result.status();
}

/**
 * @brief RAII 风格的监听
 */
inline Result<ScopedListener> bind(const std::string& address, const ListenOptions& opt = {}) {
  auto result = listen(address, opt);
  if (result.ok()) {
    return ScopedListener(std::move(result).value());
  }
  return result.status();
}

}  // namespace duct::raii
