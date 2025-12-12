#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace duct {

class Message {
 public:
  Message() = default;

  static Message from_bytes(const void* data, size_t size);
  static Message from_string(std::string_view s);

  const std::uint8_t* data() const { return data_; }
  std::uint8_t* data() { return data_; }
  size_t size() const { return size_; }

  // Ownership: shared backing store to enable cheap copies and future zero-copy.
  const std::shared_ptr<std::vector<std::uint8_t>>& backing() const { return backing_; }

 private:
  std::shared_ptr<std::vector<std::uint8_t>> backing_;
  std::uint8_t* data_ = nullptr;
  size_t size_ = 0;
};

}  // namespace duct

