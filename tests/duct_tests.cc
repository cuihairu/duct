#include "duct/duct.h"
#include "duct/wire.h"

#include <chrono>
#include <future>
#include <iostream>
#include <string>
#include <thread>

#if !defined(_WIN32)
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace {

int g_failures = 0;

void fail(const char* file, int line, const std::string& msg) {
  std::cerr << file << ":" << line << ": " << msg << "\n";
  ++g_failures;
}

#define EXPECT_TRUE(x)                            \
  do {                                            \
    if (!(x)) fail(__FILE__, __LINE__, #x);       \
  } while (0)

#define EXPECT_EQ(a, b)                                                   \
  do {                                                                    \
    auto _a = (a);                                                        \
    auto _b = (b);                                                        \
    if (!((_a) == (_b))) {                                                \
      fail(__FILE__, __LINE__, std::string(#a) + " != " + std::string(#b)); \
    }                                                                     \
  } while (0)

static void test_address_parse() {
  {
    auto a = duct::Address::parse("127.0.0.1:1234");
    EXPECT_TRUE(a.ok());
    EXPECT_EQ(a.value().scheme, duct::Scheme::kTcp);
    EXPECT_EQ(a.value().tcp.host, "127.0.0.1");
    EXPECT_EQ(a.value().tcp.port, 1234);
  }
  {
    auto a = duct::Address::parse("tcp://:9");
    EXPECT_TRUE(a.ok());
    EXPECT_EQ(a.value().tcp.host, "127.0.0.1");
    EXPECT_EQ(a.value().tcp.port, 9);
  }
  {
    auto a = duct::Address::parse("shm://gamebus");
    EXPECT_TRUE(a.ok());
    EXPECT_EQ(a.value().scheme, duct::Scheme::kShm);
    EXPECT_EQ(a.value().name, "gamebus");
  }
}

static void test_shm_echo_one() {
  auto lis_r = duct::listen("shm://duct_testbus");
  EXPECT_TRUE(lis_r.ok());
  if (!lis_r.ok()) return;

  auto server = std::async(std::launch::async, [&]() -> duct::Result<void> {
    auto p = lis_r.value()->accept();
    if (!p.ok()) return p.status();
    auto msg = p.value()->recv({});
    if (!msg.ok()) return msg.status();
    return p.value()->send(msg.value(), {});
  });

  // Give the listener a moment to bind the bootstrap socket path.
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  auto c = duct::dial("shm://duct_testbus");
  EXPECT_TRUE(c.ok());
  if (!c.ok()) return;

  auto st = c.value()->send(duct::Message::from_string("hello"), {});
  EXPECT_TRUE(st.ok());
  if (!st.ok()) return;

  auto echoed = c.value()->recv({});
  EXPECT_TRUE(echoed.ok());
  if (!echoed.ok()) return;

  std::string s(reinterpret_cast<const char*>(echoed.value().data()), echoed.value().size());
  EXPECT_EQ(s, "hello");

  auto sst = server.wait_for(std::chrono::seconds(1));
  EXPECT_TRUE(sst == std::future_status::ready);
  if (sst == std::future_status::ready) {
    auto sr = server.get();
    EXPECT_TRUE(sr.ok());
  }

  lis_r.value()->close();
}

static void test_shm_backpressure_timeout() {
  auto lis_r = duct::listen("shm://duct_testbp");
  EXPECT_TRUE(lis_r.ok());
  if (!lis_r.ok()) return;

  auto accepted = std::promise<duct::Result<std::unique_ptr<duct::Pipe>>>();
  auto fut = accepted.get_future();
  std::thread t([&] { accepted.set_value(lis_r.value()->accept()); });

  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  auto c = duct::dial("shm://duct_testbp");
  EXPECT_TRUE(c.ok());
  if (!c.ok()) {
    t.join();
    return;
  }

  auto sr = fut.get();
  EXPECT_TRUE(sr.ok());
  if (!sr.ok()) {
    t.join();
    return;
  }

  // Do not recv on server side: fill client's TX ring until it blocks.
  (void)sr.value();

  duct::SendOptions opt;
  opt.timeout = std::chrono::milliseconds(50);

  bool saw_timeout = false;
  for (int i = 0; i < 256; ++i) {
    auto st = c.value()->send(duct::Message::from_string("x"), opt);
    if (!st.ok()) {
      EXPECT_EQ(st.status().code(), duct::StatusCode::kTimeout);
      saw_timeout = true;
      break;
    }
  }
  EXPECT_TRUE(saw_timeout);

  lis_r.value()->close();
  t.join();
}

static void test_wire_decode_rejects_bad_magic() {
  std::uint8_t hdr[duct::wire::kHeaderLen]{};
  auto decoded = duct::wire::decode_header(hdr);
  EXPECT_TRUE(!decoded.ok());
  if (!decoded.ok()) {
    EXPECT_EQ(decoded.status().code(), duct::StatusCode::kProtocolError);
  }
}

static void test_wire_socketpair_frames() {
#if defined(_WIN32)
  // TODO: Windows tests can use a loopback TCP socket or a named pipe.
  return;
#else
  int fds[2]{-1, -1};
  int rc = ::socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
  EXPECT_TRUE(rc == 0);
  if (rc != 0) return;

  auto writer = std::thread([&] {
    (void)duct::wire::write_frame(fds[0], duct::Message::from_string("one"));
    (void)duct::wire::write_frame(fds[0], duct::Message::from_string("two"));

    // Max payload frame.
    std::string big(duct::wire::kMaxFramePayload, 'x');
    (void)duct::wire::write_frame(fds[0], duct::Message::from_string(big));
    ::close(fds[0]);
  });

  auto r1 = duct::wire::read_frame(fds[1]);
  EXPECT_TRUE(r1.ok());
  if (r1.ok()) {
    std::string s(reinterpret_cast<const char*>(r1.value().data()), r1.value().size());
    EXPECT_EQ(s, "one");
  }

  auto r2 = duct::wire::read_frame(fds[1]);
  EXPECT_TRUE(r2.ok());
  if (r2.ok()) {
    std::string s(reinterpret_cast<const char*>(r2.value().data()), r2.value().size());
    EXPECT_EQ(s, "two");
  }

  auto r3 = duct::wire::read_frame(fds[1]);
  EXPECT_TRUE(r3.ok());
  if (r3.ok()) {
    EXPECT_EQ(r3.value().size(), duct::wire::kMaxFramePayload);
  }

  ::close(fds[1]);
  writer.join();
#endif
}

}  // namespace

int main() {
  test_address_parse();
  test_shm_echo_one();
  test_shm_backpressure_timeout();
  test_wire_decode_rejects_bad_magic();
  test_wire_socketpair_frames();

  if (g_failures != 0) {
    std::cerr << "FAIL (" << g_failures << ")\n";
    return 1;
  }
  std::cerr << "OK\n";
  return 0;
}
