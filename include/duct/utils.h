#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <type_traits>

#include "duct/duct.h"

namespace duct::utils {

// ==============================================================================
// 类型安全的地址构建器
// ==============================================================================

/**
 * @brief 强类型端口，避免混淆
 */
class Port {
 public:
  explicit constexpr Port(std::uint16_t value) : value_(value) {}
  constexpr std::uint16_t value() const { return value_; }

  constexpr bool operator==(const Port& other) const { return value_ == other.value_; }
  constexpr bool operator!=(const Port& other) const { return value_ != other.value_; }

 private:
  std::uint16_t value_;
};

/**
 * @brief TCP 地址构建器（工具版本）
 */
class TcpAddr {
 public:
  constexpr TcpAddr(std::string_view host, Port port)
      : host_(host), port_(port) {}

  // 构建地址字符串
  std::string build() const {
    return "tcp://" + std::string(host_) + ":" + std::to_string(port_.value());
  }

  // 隐式转换
  operator std::string() const { return build(); }

  constexpr std::string_view host() const { return host_; }
  constexpr Port port() const { return port_; }

 private:
  std::string_view host_;
  Port port_;
};

/**
 * @brief 共享内存地址构建器（工具版本）
 */
class ShmAddr {
 public:
  explicit constexpr ShmAddr(std::string_view name) : name_(name) {}

  std::string build() const {
    return "shm://" + std::string(name_);
  }

  operator std::string() const { return build(); }

  constexpr std::string_view name() const { return name_; }

 private:
  std::string_view name_;
};

/**
 * @brief Unix 域套接字地址构建器（工具版本）
 */
class UdsAddr {
 public:
  explicit constexpr UdsAddr(std::string_view path) : path_(path) {}

  std::string build() const {
    return "uds://" + std::string(path_);
  }

  operator std::string() const { return build(); }

  constexpr std::string_view path() const { return path_; }

 private:
  std::string_view path_;
};

// 便捷字面量运算符
namespace literals {
  constexpr Port operator""_port(unsigned long long value) {
    return Port(static_cast<std::uint16_t>(value));
  }
}  // namespace literals

// ==============================================================================
// 时间工具
// ==============================================================================

namespace time {

// 便捷时间字面量
namespace literals {
  constexpr std::chrono::milliseconds operator"" _ms(unsigned long long value) {
    return std::chrono::milliseconds(value);
  }
  constexpr std::chrono::seconds operator"" _s(unsigned long long value) {
    return std::chrono::seconds(value);
  }
  constexpr std::chrono::minutes operator"" _min(unsigned long long value) {
    return std::chrono::minutes(value);
  }
  constexpr std::chrono::hours operator"" _h(unsigned long long value) {
    return std::chrono::hours(value);
  }
}  // namespace literals

/**
 * @brief 简单的计时器
 */
class Timer {
 public:
  using Clock = std::chrono::steady_clock;
  using TimePoint = Clock::time_point;
  using Duration = Clock::duration;

  Timer() : start_(Clock::now()) {}

  // 重置计时器
  void reset() { start_ = Clock::now(); }

  // 获取经过的时间
  template <class Duration = std::chrono::milliseconds>
  Duration elapsed() const {
    return std::chrono::duration_cast<Duration>(Clock::now() - start_);
  }

  // 检查是否超时
  template <class Duration>
  bool has_elapsed(Duration duration) const {
    return elapsed<std::chrono::nanoseconds>() >= duration;
  }

 private:
  TimePoint start_;
};

/**
 * @brief 周期性执行器
 */
class PeriodicExecutor {
 public:
  using Duration = std::chrono::milliseconds;

  PeriodicExecutor(Duration interval, std::function<void()> callback)
      : interval_(interval), callback_(std::move(callback)) {}

  // 立即执行一次
  void execute_now() const {
    if (callback_) callback_();
  }

  // 检查是否需要执行
  bool tick() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = now - last_exec_;

    if (elapsed >= interval_) {
      if (callback_) callback_();
      last_exec_ = now;
      return true;
    }
    return false;
  }

  Duration interval() const { return interval_; }

 private:
  Duration interval_;
  std::function<void()> callback_;
  std::chrono::steady_clock::time_point last_exec_ = std::chrono::steady_clock::now();
};

}  // namespace time

// ==============================================================================
// 回调工具
// ==============================================================================

/**
 * @brief 作用域守卫 - 在作用域结束时执行指定操作
 */
template <class F>
class ScopeGuard {
 public:
  explicit ScopeGuard(F&& f) : f_(std::forward<F>(f)), active_(true) {}
  ~ScopeGuard() {
    if (active_) f_();
  }

  // 禁止拷贝
  ScopeGuard(const ScopeGuard&) = delete;
  ScopeGuard& operator=(const ScopeGuard&) = delete;

  // 支持移动
  ScopeGuard(ScopeGuard&& other) noexcept
      : f_(std::move(other.f_)), active_(other.active_) {
    other.active_ = false;
  }

  // 提前触发
  void trigger() {
    if (active_) {
      f_();
      active_ = false;
    }
  }

  // 取消
  void cancel() { active_ = false; }

 private:
  F f_;
  bool active_;
};

/**
 * @brief 创建作用域守卫的便捷函数
 */
template <class F>
ScopeGuard<std::decay_t<F>> make_scope_guard(F&& f) {
  return ScopeGuard<std::decay_t<F>>(std::forward<F>(f));
}

// ==============================================================================
// 字符串工具
// ==============================================================================

/**
 * @brief 字符串拼接器
 */
class StringBuilder {
 public:
  StringBuilder& append(std::string_view s) {
    buffer_ += s;
    return *this;
  }

  // 只保留 string_view 版本，避免歧义
  StringBuilder& append(const char* s) {
    buffer_ += s;
    return *this;
  }

  StringBuilder& append(int value) {
    buffer_ += std::to_string(value);
    return *this;
  }

  StringBuilder& append(unsigned int value) {
    buffer_ += std::to_string(value);
    return *this;
  }

  StringBuilder& append(long value) {
    buffer_ += std::to_string(value);
    return *this;
  }

  StringBuilder& append(long long value) {
    buffer_ += std::to_string(value);
    return *this;
  }

  StringBuilder& append(unsigned long value) {
    buffer_ += std::to_string(value);
    return *this;
  }

  StringBuilder& append(unsigned long long value) {
    buffer_ += std::to_string(value);
    return *this;
  }

  StringBuilder& append(float value) {
    buffer_ += std::to_string(value);
    return *this;
  }

  StringBuilder& append(double value) {
    buffer_ += std::to_string(value);
    return *this;
  }

  std::string build() const { return buffer_; }
  std::string_view view() const { return buffer_; }

  void clear() { buffer_.clear(); }
  size_t size() const { return buffer_.size(); }
  bool empty() const { return buffer_.empty(); }

 private:
  std::string buffer_;
};

// ==============================================================================
// 函数工具
// ==============================================================================

/**
 * @brief 延迟调用 - 在对象析构时调用函数（类似 Go 的 defer）
 */
template <class F>
class Defer {
 public:
  explicit Defer(F&& f) : f_(std::forward<F>(f)) {}
  ~Defer() { f_(); }

  Defer(const Defer&) = delete;
  Defer& operator=(const Defer&) = delete;

 private:
  F f_;
};

/**
 * @brief 创建延迟调用的便捷函数
 */
template <class F>
Defer<std::decay_t<F>> defer(F&& f) {
  return Defer<std::decay_t<F>>(std::forward<F>(f));
}

// ==============================================================================
// 比较工具
// ==============================================================================

/**
 * @brief 强类型包装器，用于区分相同底层类型的值
 */
template <class T, class Tag>
class StrongType {
 public:
  explicit constexpr StrongType(T value) : value_(std::move(value)) {}

  constexpr const T& value() const { return value_; }
  T& value() { return value_; }

  constexpr bool operator==(const StrongType& other) const {
    return value_ == other.value_;
  }
  constexpr bool operator!=(const StrongType& other) const {
    return value_ != other.value_;
  }
  constexpr bool operator<(const StrongType& other) const {
    return value_ < other.value_;
  }
  constexpr bool operator<=(const StrongType& other) const {
    return value_ <= other.value_;
  }
  constexpr bool operator>(const StrongType& other) const {
    return value_ > other.value_;
  }
  constexpr bool operator>=(const StrongType& other) const {
    return value_ >= other.value_;
  }

 private:
  T value_;
};

}  // namespace duct::utils
