// 工具类使用示例
// 展示如何使用 duct::utils 和 duct::logging 命名空间下的工具

#include "duct/logging.h"
#include "duct/utils.h"

#include <iostream>
#include <thread>

using namespace duct;
using namespace duct::utils;
using namespace duct::utils::literals;
using namespace duct::utils::time;
using namespace duct::utils::time::literals;

// 示例 1: 类型安全的地址构建器
void example_type_safe_address() {
  std::cout << "=== 示例 1: 类型安全的地址构建器 ===" << std::endl;

  // 使用强类型端口
  TcpAddr addr1("127.0.0.1", 9000_port);
  std::cout << "TCP 地址: " << addr1.build() << std::endl;

  // 使用字面量运算符
  TcpAddr addr2("localhost", 8080_port);
  std::cout << "TCP 地址: " << std::string(addr2) << std::endl;

  // 共享内存地址
  ShmAddr shm("gamebus");
  std::cout << "SHM 地址: " << shm.build() << std::endl;

  // Unix 域套接字地址
  UdsAddr uds("/tmp/mysocket");
  std::cout << "UDS 地址: " << uds.build() << std::endl;
}

// 示例 2: 计时器
void example_timer() {
  std::cout << "\n=== 示例 2: 计时器 ===" << std::endl;

  Timer timer;

  // 模拟一些工作
  std::this_thread::sleep_for(100_ms);

  std::cout << "经过时间: " << timer.elapsed<std::chrono::milliseconds>().count()
            << " ms" << std::endl;

  std::this_thread::sleep_for(50_ms);

  // 检查是否超过指定时间
  if (timer.has_elapsed(150_ms)) {
    std::cout << "已超过 150ms" << std::endl;
  }
}

// 示例 3: 作用域守卫
void example_scope_guard() {
  std::cout << "\n=== 示例 3: 作用域守卫 ===" << std::endl;

  {
    auto guard = make_scope_guard([]() {
      std::cout << "作用域结束，执行清理..." << std::endl;
    });

    std::cout << "在作用域内工作" << std::endl;
  }  // guard 在这里自动触发

  std::cout << "已离开作用域" << std::endl;
}

// 示例 4: 延迟调用（defer）
void example_defer() {
  std::cout << "\n=== 示例 4: 延迟调用 ===" << std::endl;

  auto cleanup = defer([]() {
    std::cout << "延迟执行：清理资源" << std::endl;
  });

  std::cout << "执行一些操作..." << std::endl;
  // cleanup 在这里自动执行
}

// 示例 5: 字符串构建器
void example_string_builder() {
  std::cout << "\n=== 示例 5: 字符串构建器 ===" << std::endl;

  StringBuilder builder;
  builder.append("Hello, ")
          .append("duct")
          .append("! Version: ")
          .append(0)
          .append(".")
          .append(1)
          .append(".")
          .append(0);

  std::cout << builder.build() << std::endl;
}

// 示例 6: 强类型包装器
void example_strong_type() {
  std::cout << "\n=== 示例 6: 强类型包装器 ===" << std::endl;

  // 定义用户 ID 类型
  using UserId = StrongType<int, struct UserIdTag>;

  UserId user1(123);
  UserId user2(456);

  if (user1 < user2) {
    std::cout << "user1 < user2" << std::endl;
  }
}

// 示例 7: 日志功能
void example_logging() {
  std::cout << "\n=== 示例 7: 日志功能 ===" << std::endl;

  // 设置日志级别
  set_log_level(LogLevel::kDebug);

  // 使用全局日志函数
  trace("这是 trace 消息");
  debug("这是 debug 消息");
  info("这是 info 消息");
  warning("这是 warning 消息");
  error("这是 error 消息");

  // 使用宏
  DUCT_INFO("使用宏记录日志");

  // 使用流式日志
  DUCT_LOG_STREAM(kInfo) << "流式日志: " << 42 << ", " << 3.14;

  // 创建带前缀的日志记录器
  auto prefixed_logger = std::make_shared<PrefixLogger>(
      std::make_shared<ConsoleLogger>(),
      "[MyApp] ");

  set_logger(prefixed_logger);
  DUCT_INFO("使用带前缀的日志记录器");

  // 恢复默认日志
  set_logger(std::make_shared<ConsoleLogger>());
}

// 示例 8: 指标收集
void example_metrics() {
  std::cout << "\n=== 示例 8: 指标收集 ===" << std::endl;

  auto& registry = MetricRegistry::instance();

  // 获取或创建计数器
  auto requests_total = registry.get_counter("requests_total");
  requests_total->increment();
  requests_total->increment();
  std::cout << "请求总数: " << requests_total->value() << std::endl;

  // 获取或创建仪表
  auto active_connections = registry.get_gauge("active_connections");
  active_connections->set(10);
  active_connections->increment();  // 11
  active_connections->decrement();  // 10
  std::cout << "活跃连接: " << active_connections->value() << std::endl;

  // 获取或创建直方图
  auto request_duration = registry.get_histogram("request_duration_ms");
  request_duration->observe(10.5);
  request_duration->observe(20.3);
  request_duration->observe(15.7);
  std::cout << "请求数: " << request_duration->count() << std::endl;
  std::cout << "平均耗时: " << request_duration->mean() << " ms" << std::endl;
}

// 示例 9: 周期性执行器
void example_periodic_executor() {
  std::cout << "\n=== 示例 9: 周期性执行器 ===" << std::endl;

  int counter = 0;
  PeriodicExecutor executor(100_ms, [&counter]() {
    counter++;
    std::cout << "周期执行: 第 " << counter << " 次" << std::endl;
  });

  // 模拟多次 tick
  for (int i = 0; i < 3; ++i) {
    std::this_thread::sleep_for(150_ms);  // 等待超过周期
    executor.tick();
  }
}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "用法: " << argv[0] << " <example_number>\n"
              << "示例:\n"
              << "  1 - 类型安全的地址构建器\n"
              << "  2 - 计时器\n"
              << "  3 - 作用域守卫\n"
              << "  4 - 延迟调用\n"
              << "  5 - 字符串构建器\n"
              << "  6 - 强类型包装器\n"
              << "  7 - 日志功能\n"
              << "  8 - 指标收集\n"
              << "  9 - 周期性执行器\n";
    return 2;
  }

  int example = std::stoi(argv[1]);

  switch (example) {
    case 1: example_type_safe_address(); break;
    case 2: example_timer(); break;
    case 3: example_scope_guard(); break;
    case 4: example_defer(); break;
    case 5: example_string_builder(); break;
    case 6: example_strong_type(); break;
    case 7: example_logging(); break;
    case 8: example_metrics(); break;
    case 9: example_periodic_executor(); break;
    default:
      std::cerr << "无效的示例编号: " << example << std::endl;
      return 1;
  }

  return 0;
}
