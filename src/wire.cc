#include "duct/wire.h"

#include <cstdint>
#include <cstring>

#include "duct/protocol.h"

#if !defined(_WIN32)
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace duct::wire {

void encode_header(const FrameHeader& h, std::uint8_t out[kHeaderLen]) {
#if defined(_WIN32)
  (void)h;
  (void)out;
#else
  std::uint32_t magic = htonl(h.magic);
  std::uint16_t version = htons(h.version);
  std::uint16_t header_len = htons(h.header_len);
  std::uint32_t payload_len = htonl(h.payload_len);
  std::uint32_t flags = htonl(h.flags);

  std::memcpy(out + 0, &magic, 4);
  std::memcpy(out + 4, &version, 2);
  std::memcpy(out + 6, &header_len, 2);
  std::memcpy(out + 8, &payload_len, 4);
  std::memcpy(out + 12, &flags, 4);
#endif
}

Result<FrameHeader> decode_header(const std::uint8_t in[kHeaderLen]) {
#if defined(_WIN32)
  (void)in;
  return Status::not_supported("wire decode not implemented on windows yet");
#else
  FrameHeader h;
  std::uint32_t magic = 0;
  std::uint16_t version = 0;
  std::uint16_t header_len = 0;
  std::uint32_t payload_len = 0;
  std::uint32_t flags = 0;

  std::memcpy(&magic, in + 0, 4);
  std::memcpy(&version, in + 4, 2);
  std::memcpy(&header_len, in + 6, 2);
  std::memcpy(&payload_len, in + 8, 4);
  std::memcpy(&flags, in + 12, 4);

  h.magic = ntohl(magic);
  h.version = ntohs(version);
  h.header_len = ntohs(header_len);
  h.payload_len = ntohl(payload_len);
  h.flags = ntohl(flags);

  if (h.magic != kProtocolMagic) {
    return Status::protocol_error("bad magic");
  }
  if (h.version != kProtocolVersion) {
    return Status::protocol_error("unsupported version");
  }
  if (h.header_len != kHeaderLen) {
    return Status::protocol_error("bad header_len");
  }
  if (h.payload_len > kMaxFramePayload) {
    return Status::protocol_error("payload too large (frame)");
  }
  return h;
#endif
}

#if !defined(_WIN32)
namespace {

Result<void> write_all(int fd, const std::uint8_t* p, std::size_t n) {
  while (n != 0) {
    ssize_t w = ::send(fd, p, n, 0);
    if (w < 0) {
      if (errno == EINTR) continue;
      return Status::io_error("send() failed");
    }
    if (w == 0) return Status::closed("peer closed");
    p += static_cast<std::size_t>(w);
    n -= static_cast<std::size_t>(w);
  }
  return {};
}

Result<void> read_exact(int fd, std::uint8_t* p, std::size_t n) {
  while (n != 0) {
    ssize_t r = ::recv(fd, p, n, 0);
    if (r < 0) {
      if (errno == EINTR) continue;
      return Status::io_error("recv() failed");
    }
    if (r == 0) return Status::closed("peer closed");
    p += static_cast<std::size_t>(r);
    n -= static_cast<std::size_t>(r);
  }
  return {};
}

}  // namespace

Result<void> write_frame(int fd, const Message& msg, std::uint32_t flags) {
  if (msg.size() > kMaxFramePayload) {
    return Status::invalid_argument("message too large; enable fragmentation (todo)");
  }

  FrameHeader h;
  h.magic = kProtocolMagic;
  h.version = kProtocolVersion;
  h.header_len = static_cast<std::uint16_t>(kHeaderLen);
  h.payload_len = static_cast<std::uint32_t>(msg.size());
  h.flags = flags;

  std::uint8_t hdr[kHeaderLen];
  encode_header(h, hdr);
  auto st = write_all(fd, hdr, sizeof(hdr));
  if (!st.ok()) return st;
  return write_all(fd, msg.data(), msg.size());
}

Result<Message> read_frame(int fd) {
  std::uint8_t hdr[kHeaderLen];
  auto st = read_exact(fd, hdr, sizeof(hdr));
  if (!st.ok()) return st.status();
  auto decoded = decode_header(hdr);
  if (!decoded.ok()) return decoded.status();

  FrameHeader h = decoded.value();
  std::vector<std::uint8_t> buf(h.payload_len);
  if (h.payload_len != 0) {
    st = read_exact(fd, buf.data(), buf.size());
    if (!st.ok()) return st.status();
  }
  return Message::from_bytes(buf.data(), buf.size());
}
#endif

}  // namespace duct::wire

