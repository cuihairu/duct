#include "duct/message.h"

#include <cstring>

namespace duct {

Message Message::from_bytes(const void* data, size_t size) {
  Message m;
  m.backing_ = std::make_shared<std::vector<std::uint8_t>>(size);
  if (size != 0) {
    std::memcpy(m.backing_->data(), data, size);
  }
  m.data_ = m.backing_->data();
  m.size_ = size;
  return m;
}

Message Message::from_string(std::string_view s) {
  return from_bytes(s.data(), s.size());
}

}  // namespace duct

