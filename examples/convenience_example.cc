// 便捷 API 使用示例
// 展示如何使用 duct::convenience 命名空间下的便捷 API

#include "duct/convenience.h"
#include "duct/raii.h"

#include <iostream>
#include <string>
#include <thread>

using namespace duct;
using namespace duct::convenience;
using namespace duct::raii;

// 示例 1: 使用构建器 API 创建连接
void example_builder_api() {
  std::cout << "=== 示例 1: 构建器 API ===" << std::endl;

  // 方式 1: 使用链式构建器
  auto pipe = tcp("127.0.0.1", 9000)
      .timeout(std::chrono::seconds(5))
      .send_hwm(8 * 1024 * 1024)
      .auto_reconnect()
      .connect();

  if (!pipe.ok()) {
    std::cerr << "连接失败: " << pipe.status().to_string() << std::endl;
    return;
  }

  std::cout << "已连接到服务器" << std::endl;
}

// 示例 2: 使用 RAII 风格的资源管理
void example_raii() {
  std::cout << "\n=== 示例 2: RAII 风格 ===" << std::endl;

  // 使用 RAII 包装器，自动管理资源
  auto result = raii::connect("tcp://127.0.0.1:9000");
  if (!result.ok()) {
    std::cerr << "连接失败: " << result.status().to_string() << std::endl;
    return;
  }

  ScopedPipe pipe = std::move(result).value();

  // 发送消息
  auto msg = Message::from_string("Hello, duct!");
  auto st = pipe.send(msg, {});
  if (!st.ok()) {
    std::cerr << "发送失败: " << st.status().to_string() << std::endl;
  }

  // pipe 在作用域结束时自动关闭
}

// 示例 3: 请求-响应模式
void example_request_response() {
  std::cout << "\n=== 示例 3: 请求-响应模式 ===" << std::endl;

  auto result = raii::connect("tcp://127.0.0.1:9000");
  if (!result.ok()) {
    std::cerr << "连接失败: " << result.status().to_string() << std::endl;
    return;
  }

  ScopedPipe pipe = std::move(result).value();

  // 发送请求并等待响应
  auto req = Message::from_string("ping");
  auto response = convenience::request(*pipe.get(), req, std::chrono::seconds(5));

  if (response.ok()) {
    std::cout << "收到响应: " << std::string_view(
        reinterpret_cast<const char*>(response.value().data()),
        response.value().size()) << std::endl;
  } else {
    std::cerr << "请求失败: " << response.status().to_string() << std::endl;
  }
}

// 示例 4: 批量操作
void example_batch_operations() {
  std::cout << "\n=== 示例 4: 批量操作 ===" << std::endl;

  auto result = raii::connect("tcp://127.0.0.1:9000");
  if (!result.ok()) {
    std::cerr << "连接失败: " << result.status().to_string() << std::endl;
    return;
  }

  ScopedPipe pipe = std::move(result).value();

  // 批量发送
  std::vector<Message> messages;
  for (int i = 0; i < 10; ++i) {
    messages.push_back(Message::from_string("message " + std::to_string(i)));
  }

  auto sent = convenience::send_batch(*pipe.get(), messages);
  if (sent.ok()) {
    std::cout << "成功发送 " << sent.value() << " 条消息" << std::endl;
  }
}

// 示例 5: 使用回调处理器
void example_callback_handler() {
  std::cout << "\n=== 示例 5: 回调处理器 ===" << std::endl;

  auto result = convenience::connect_raw("tcp://127.0.0.1:9000");
  if (!result.ok()) {
    std::cerr << "连接失败: " << result.status().to_string() << std::endl;
    return;
  }

  auto pipe_ptr = std::move(result).value();

  // 使用 serve 函数处理消息
  convenience::serve(std::move(pipe_ptr), [](Message& msg) {
    std::cout << "收到消息: " << std::string_view(
        reinterpret_cast<const char*>(msg.data()),
        msg.size()) << std::endl;

    // 处理消息...
    return Status::Ok();
  });
}

// 示例 6: 使用异常风格的 API
void example_exception_style() {
  std::cout << "\n=== 示例 6: 异常风格 ===" << std::endl;

  try {
    // 使用 value_or_throw() 在失败时抛出异常
    auto result = dial("tcp://127.0.0.1:9000");
    // value_or_throw() 返回右值引用，直接构造
    ScopedPipe pipe(std::move(result.value_or_throw()));

    auto msg = Message::from_string("Hello!");
    pipe.send(msg, {}).status().throw_if_error();

    std::cout << "消息发送成功" << std::endl;

  } catch (const Exception& e) {
    std::cerr << "异常: " << e.what() << std::endl;
  }
}

// 示例 7: Echo 服务器
void example_echo_server() {
  std::cout << "\n=== 示例 7: Echo 服务器 ===" << std::endl;

  auto result = raii::bind("tcp://127.0.0.1:9000");
  if (!result.ok()) {
    std::cerr << "监听失败: " << result.status().to_string() << std::endl;
    return;
  }

  ScopedListener listener = std::move(result).value();

  std::cout << "等待连接..." << std::endl;

  auto pipe_result = listener.accept();
  if (!pipe_result.ok()) {
    std::cerr << "接受连接失败: " << pipe_result.status().to_string() << std::endl;
    return;
  }

  ScopedPipe pipe = std::move(pipe_result).value();
  std::cout << "客户端已连接" << std::endl;

  // 运行 echo 服务器
  auto st = convenience::echo_server(std::move(pipe).release());
  if (!st.ok()) {
    std::cerr << "Echo 服务器错误: " << st.status().to_string() << std::endl;
  }
}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "用法: " << argv[0] << " <example_number>\n"
              << "示例:\n"
              << "  1 - 构建器 API\n"
              << "  2 - RAII 风格\n"
              << "  3 - 请求-响应模式\n"
              << "  4 - 批量操作\n"
              << "  5 - 回调处理器\n"
              << "  6 - 异常风格\n"
              << "  7 - Echo 服务器\n";
    return 2;
  }

  int example = std::stoi(argv[1]);

  switch (example) {
    case 1: example_builder_api(); break;
    case 2: example_raii(); break;
    case 3: example_request_response(); break;
    case 4: example_batch_operations(); break;
    case 5: example_callback_handler(); break;
    case 6: example_exception_style(); break;
    case 7: example_echo_server(); break;
    default:
      std::cerr << "无效的示例编号: " << example << std::endl;
      return 1;
  }

  return 0;
}
