#pragma once

#include <cstddef>
#include <cstdint>

#include "duct/message.h"
#include "duct/status.h"

namespace duct::wire {

constexpr std::size_t kHeaderLen = 16;
constexpr std::size_t kMaxFramePayload = 64 * 1024;

struct FrameHeader {
  std::uint32_t magic = 0;
  std::uint16_t version = 0;
  std::uint16_t header_len = 0;
  std::uint32_t payload_len = 0;
  std::uint32_t flags = 0;
};

// Encoding/decoding uses network byte order for integral fields.
void encode_header(const FrameHeader& h, std::uint8_t out[kHeaderLen]);
Result<FrameHeader> decode_header(const std::uint8_t in[kHeaderLen]);

#if !defined(_WIN32)
Result<void> write_frame(int fd, const Message& msg, std::uint32_t flags = 0);
Result<Message> read_frame(int fd);
#endif

}  // namespace duct::wire

