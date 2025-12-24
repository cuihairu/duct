// 异步 API 使用示例
// 展示如何使用 duct::async 命名空间下的异步 API

#include "duct/async.h"
#include "duct/convenience.h"
#include "duct/raii.h"

#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace duct;
using namespace duct::async;
using namespace duct::convenience;
using namespace duct::raii;

// 示例 1: 使用 Future 风格的异步操作
void example_async_future() {
  std::cout << "=== 示例 1: Future 风格的异步操作 ===" << std::endl;

  // 异步连接
  auto connect_result = async_dial("tcp://127.0.0.1:9000");

  // 等待连接完成
  try {
    auto pipe = connect_result.get();
    std::cout << "已连接到服务器" << std::endl;

    // 异步发送
    auto msg = Message::from_string("Hello, async!");
    auto send_result = async_send(pipe, msg);
    send_result.get();  // 等待发送完成
    std::cout << "消息已发送" << std::endl;

  } catch (const Exception& e) {
    std::cerr << "错误: " << e.what() << std::endl;
  }
}

// 示例 2: 使用回调风格的异步操作
void example_async_callback() {
  std::cout << "\n=== 示例 2: 回调风格的异步操作 ===" << std::endl;

  auto result = dial("tcp://127.0.0.1:9000");
  if (!result.ok()) {
    std::cerr << "连接失败: " << result.status().to_string() << std::endl;
    return;
  }

  auto pipe = std::shared_ptr<Pipe>(result.value().release());

  // 异步发送并处理结果
  async_send(pipe, Message::from_string("Hello with callback!"),
    [](const Result<void>& result) {
      if (result.ok()) {
        std::cout << "发送成功 (回调)" << std::endl;
      } else {
        std::cerr << "发送失败: " << result.status().to_string() << std::endl;
      }
    });

  // 给异步操作一些时间完成
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// 示例 3: 使用事件循环管理多个连接
void example_event_loop() {
  std::cout << "\n=== 示例 3: 事件循环 ===" << std::endl;

  EventLoop loop;

  // 添加多个管道到事件循环
  for (int i = 0; i < 3; ++i) {
    auto result = dial("tcp://127.0.0.1:9000");
    if (!result.ok()) {
      std::cerr << "连接 " << i << " 失败" << std::endl;
      continue;
    }

    auto pipe = std::shared_ptr<Pipe>(result.value().release());

    // 为每个管道设置消息处理器
    loop.add_pipe(pipe,
      [i](const Message& msg) {
        std::cout << "管道 " << i << " 收到消息: "
                  << std::string_view(
                      reinterpret_cast<const char*>(msg.data()),
                      msg.size()) << std::endl;
      },
      [i](const Status& status) {
        std::cerr << "管道 " << i << " 错误: "
                  << status.to_string() << std::endl;
      });
  }

  std::cout << "事件循环运行中..." << std::endl;

  // 在后台线程运行事件循环
  loop.runInBackground();

  // 让它运行一段时间
  std::this_thread::sleep_for(std::chrono::seconds(5));

  // 停止事件循环
  loop.stop();
  std::cout << "事件循环已停止" << std::endl;
}

// 示例 4: 使用 Channel 进行线程间通信
void example_channel() {
  std::cout << "\n=== 示例 4: Channel 线程间通信 ===" << std::endl;

  Channel<Message> channel;

  // 生产者线程
  std::thread producer([&channel]() {
    for (int i = 0; i < 5; ++i) {
      auto msg = Message::from_string("Message " + std::to_string(i));
      channel.send(std::move(msg));
      std::cout << "生产者: 发送消息 " << i << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    channel.close();
    std::cout << "生产者: 关闭 channel" << std::endl;
  });

  // 消费者线程
  std::thread consumer([&channel]() {
    Message msg;
    int count = 0;
    while (channel.recv(msg)) {
      std::cout << "消费者: 收到消息: "
                << std::string_view(
                    reinterpret_cast<const char*>(msg.data()),
                    msg.size()) << std::endl;
      ++count;
    }
    std::cout << "消费者: 收到 " << count << " 条消息后退出" << std::endl;
  });

  producer.join();
  consumer.join();
}

// 示例 5: 在后台运行 echo 服务器
void example_background_server() {
  std::cout << "\n=== 示例 5: 后台服务器 ===" << std::endl;

  // 在后台启动服务器
  auto stop_server = run_echo_serverInBackground("tcp://127.0.0.1:9001");
  std::cout << "服务器已在后台启动" << std::endl;

  // 等待服务器启动
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // 连接到服务器
  auto result = dial("tcp://127.0.0.1:9001");
  if (!result.ok()) {
    std::cerr << "连接失败: " << result.status().to_string() << std::endl;
    stop_server();
    return;
  }

  raii::ScopedPipe pipe(std::move(result).value());

  // 发送一些消息
  for (int i = 0; i < 3; ++i) {
    auto msg = Message::from_string("Echo " + std::to_string(i));
    pipe.send(msg, {});

    auto echo = pipe.recv({});
    if (echo.ok()) {
      std::cout << "收到回显: "
                << std::string_view(
                    reinterpret_cast<const char*>(echo.value().data()),
                    echo.value().size()) << std::endl;
    }
  }

  // 停止服务器
  stop_server();
  std::cout << "服务器已停止" << std::endl;
}

// 示例 6: 批量异步操作
void example_batch_async() {
  std::cout << "\n=== 示例 6: 批量异步操作 ===" << std::endl;

  auto result = dial("tcp://127.0.0.1:9000");
  if (!result.ok()) {
    std::cerr << "连接失败: " << result.status().to_string() << std::endl;
    return;
  }

  auto pipe = std::shared_ptr<Pipe>(result.value().release());

  // 启动多个异步发送
  std::vector<AsyncResult<void>> results;
  for (int i = 0; i < 10; ++i) {
    auto msg = Message::from_string("Async message " + std::to_string(i));
    results.push_back(async_send(pipe, msg));
  }

  // 等待所有发送完成
  for (size_t i = 0; i < results.size(); ++i) {
    results[i].get();
    std::cout << "消息 " << i << " 发送完成" << std::endl;
  }

  std::cout << "所有消息发送完成" << std::endl;
}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "用法: " << argv[0] << " <example_number>\n"
              << "示例:\n"
              << "  1 - Future 风格异步操作\n"
              << "  2 - 回调风格异步操作\n"
              << "  3 - 事件循环\n"
              << "  4 - Channel 线程间通信\n"
              << "  5 - 后台服务器\n"
              << "  6 - 批量异步操作\n";
    return 2;
  }

  int example = std::stoi(argv[1]);

  switch (example) {
    case 1: example_async_future(); break;
    case 2: example_async_callback(); break;
    case 3: example_event_loop(); break;
    case 4: example_channel(); break;
    case 5: example_background_server(); break;
    case 6: example_batch_async(); break;
    default:
      std::cerr << "无效的示例编号: " << example << std::endl;
      return 1;
  }

  return 0;
}
