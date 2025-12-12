#pragma once

#include <string>
#include <utility>

namespace duct {

enum class StatusCode {
  kOk = 0,
  kInvalidArgument,
  kNotSupported,
  kIoError,
  kTimeout,
  kClosed,
  kProtocolError,
};

class Status {
 public:
  Status() : code_(StatusCode::kOk) {}
  Status(StatusCode code, std::string message) : code_(code), message_(std::move(message)) {}

  static Status Ok() { return Status(); }
  static Status invalid_argument(std::string m) { return Status(StatusCode::kInvalidArgument, std::move(m)); }
  static Status not_supported(std::string m) { return Status(StatusCode::kNotSupported, std::move(m)); }
  static Status io_error(std::string m) { return Status(StatusCode::kIoError, std::move(m)); }
  static Status timeout(std::string m) { return Status(StatusCode::kTimeout, std::move(m)); }
  static Status closed(std::string m) { return Status(StatusCode::kClosed, std::move(m)); }
  static Status protocol_error(std::string m) { return Status(StatusCode::kProtocolError, std::move(m)); }

  bool ok() const { return code_ == StatusCode::kOk; }
  StatusCode code() const { return code_; }
  const std::string& message() const { return message_; }

 private:
  StatusCode code_;
  std::string message_;
};

template <class T>
class Result {
 public:
  Result(T v) : ok_(true), value_(std::move(v)), status_() {}
  Result(Status s) : ok_(false), value_(), status_(std::move(s)) {}

  bool ok() const { return ok_; }
  const Status& status() const { return status_; }

  T& value() & { return value_; }
  const T& value() const& { return value_; }
  T&& value() && { return std::move(value_); }

 private:
  bool ok_ = false;
  T value_{};
  Status status_{StatusCode::kInvalidArgument, "uninitialized"};
};

template <>
class Result<void> {
 public:
  Result() : status_(Status::Ok()) {}
  Result(Status s) : status_(std::move(s)) {}

  bool ok() const { return status_.ok(); }
  const Status& status() const { return status_; }

 private:
  Status status_;
};

}  // namespace duct
