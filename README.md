# duct

`duct` is a C++20 message-oriented networking/IPC library for low-latency server-to-server communication.

## 特性

- **多种传输协议统一抽象**
  - 共享内存（零拷贝，本地 IPC 最快）
  - Unix 域套接字（本地 IPC）
  - TCP（本地/远程网络）

- **开箱即用的功能**
  - 内置 QoS 和背压控制
  - 自动重连机制
  - 连接状态回调
  - 消息 TTL 和可靠性选项

- **便捷的 API 设计**
  - 链式构建器 API
  - RAII 风格的资源管理
  - 异步操作支持（Future 和回调风格）
  - 异常安全和错误处理
  - 类型安全的工具类

- **现代 C++20**
  - 类型安全的 API
  - 零拷贝消息传递
  - 跨平台支持

- **丰富的工具集**
  - 日志和可观察性接口
  - 指标收集（Counter、Gauge、Histogram）
  - 时间工具（计时器、周期执行器）
  - 作用域守卫和延迟调用

## 快速开始

### 基础用法

```cpp
#include "duct/duct.h"

// 连接到服务器
auto pipe = duct::dial("tcp://127.0.0.1:9000");
if (!pipe.ok()) {
  std::cerr << "连接失败: " << pipe.status().to_string() << std::endl;
  return;
}

// 发送消息
auto msg = duct::Message::from_string("Hello, duct!");
pipe.value()->send(msg, {});

// 接收消息
auto received = pipe.value()->recv({});
```

### 使用便捷 API

```cpp
#include "duct/convenience.h"

// 链式构建器 API
auto pipe = duct::tcp("127.0.0.1", 9000)
    .timeout(std::chrono::seconds(5))
    .auto_reconnect()
    .connect();

// RAII 风格
#include "duct/raii.h"
using namespace duct::raii;

auto result = connect("tcp://127.0.0.1:9000");
ScopedPipe pipe = std::move(result).value();
// pipe 自动管理资源，作用域结束时自动关闭
```

### 使用工具类

```cpp
#include "duct/utils.h"
#include "duct/logging.h"

using namespace duct::utils;
using namespace duct::utils::literals;

// 类型安全的地址构建
TcpAddr addr("localhost", 8080_port);

// 计时器
Timer timer;
std::this_thread::sleep_for(100_ms);
std::cout << "耗时: " << timer.elapsed<std::chrono::milliseconds>().count() << "ms" << std::endl;

// 作用域守卫
{
  auto guard = make_scope_guard([]() {
    std::cout << "清理资源" << std::endl;
  });
  // 做一些工作...
} // 自动触发清理

// 日志记录
using namespace duct;
set_log_level(LogLevel::kDebug);
info("应用启动");
warning("配置文件未找到，使用默认值");
```

### 异步操作

```cpp
#include "duct/async.h"

// Future 风格
auto pipe = async_dial("tcp://127.0.0.1:9000").get();

// 回调风格
async_send(pipe, msg, [](const Result<void>& result) {
  if (result.ok()) {
    std::cout << "发送成功" << std::endl;
  }
});

// 事件循环
EventLoop loop;
loop.add_pipe(pipe,
  [](const Message& msg) {
    std::cout << "收到消息" << std::endl;
  },
  [](const Status& status) {
    std::cerr << "错误: " << status.to_string() << std::endl;
  });
loop.runInBackground();
```

## 地址格式

| 协议 | 格式 | 说明 |
|------|------|------|
| TCP | `tcp://host:port` 或 `host:port` | TCP 连接 |
| 共享内存 | `shm://name` | 共享内存（本地） |
| Unix 域套接字 | `uds:///path/to/socket` | Unix 域套接字（本地） |

## 构建

### 基础构建

```bash
cmake -S . -B build
cmake --build build
```

### 构建选项

```bash
# 禁用示例
cmake -S . -B build -DDUCT_BUILD_EXAMPLES=OFF

# 禁用测试
cmake -S . -B build -DDUCT_BUILD_TESTS=OFF

# 禁用安装目标
cmake -S . -B build -DDUCT_INSTALL=OFF
```

### 安装

```bash
cmake --build build --target install
```

安装后将生成：
- 头文件到 `{CMAKE_INSTALL_INCLUDEDIR}/duct/`
- 库文件到 `{CMAKE_INSTALL_LIBDIR}/`
- CMake 配置文件到 `{CMAKE_INSTALL_LIBDIR}/cmake/duct/`
- pkg-config 文件到 `{CMAKE_INSTALL_LIBDIR}/pkgconfig/`

### 使用 CMake 集成

```cmake
find_package(duct REQUIRED)
target_link_libraries(your_target PRIVATE duct)
```

或使用 pkg-config：

```bash
pkg-config --cflags --libs duct
```

## 示例

### Echo 服务器

```bash
./build/duct_echo_server tcp://127.0.0.1:9000
```

### Echo 客户端

```bash
./build/duct_echo_client tcp://127.0.0.1:9000 "hello"
```

### 便捷 API 示例

```bash
./build/duct_convenience_example 1  # 构建器 API
./build/duct_convenience_example 2  # RAII 风格
./build/duct_convenience_example 3  # 请求-响应模式
./build/duct_convenience_example 4  # 批量操作
./build/duct_convenience_example 5  # 回调处理器
./build/duct_convenience_example 6  # 异常风格
./build/duct_convenience_example 7  # Echo 服务器
```

### 异步 API 示例

```bash
./build/duct_async_example 1  # Future 风格
./build/duct_async_example 2  # 回调风格
./build/duct_async_example 3  # 事件循环
./build/duct_async_example 4  # Channel 线程间通信
./build/duct_async_example 5  # 后台服务器
./build/duct_async_example 6  # 批量异步操作
```

### 工具类示例

```bash
./build/duct_utils_example 1  # 类型安全的地址构建器
./build/duct_utils_example 2  # 计时器
./build/duct_utils_example 3  # 作用域守卫
./build/duct_utils_example 4  # 延迟调用
./build/duct_utils_example 5  # 字符串构建器
./build/duct_utils_example 6  # 强类型包装器
./build/duct_utils_example 7  # 日志功能
./build/duct_utils_example 8  # 指标收集
./build/duct_utils_example 9  # 周期性执行器
```

## API 文档

### 核心类型

- **`duct::Message`** - 零拷贝消息类型，支持 `std::span`、字符串视图转换
- **`duct::Pipe`** - 通信管道抽象
- **`duct::Listener`** - 监听器抽象
- **`duct::Result<T>`** - 错误处理结果类型，支持 `value_or_throw()` 和 `value_or()`
- **`duct::Status`** - 状态码和错误信息，支持 `to_string()` 和 `throw_if_error()`

### 配置选项

#### QoS 选项 (`duct::QosOptions`)

```cpp
struct QosOptions {
  std::size_t snd_hwm_bytes = 4 * 1024 * 1024;  // 发送高水位
  std::size_t rcv_hwm_bytes = 4 * 1024 * 1024;  // 接收高水位
  BackpressurePolicy backpressure = kBlock;     // 背压策略
  std::chrono::milliseconds ttl{0};             // 消息 TTL
  Reliability reliability = kAtMostOnce;        // 可靠性模式
};
```

#### 重连策略 (`duct::ReconnectPolicy`)

```cpp
struct ReconnectPolicy {
  bool enabled = false;
  std::chrono::milliseconds initial_delay{100};
  std::chrono::milliseconds max_delay{30'000};
  double backoff_multiplier = 2.0;
  int max_attempts = 0;  // 0 表示无限重试
};
```

### 命名空间

- **`duct`** - 核心 API
- **`duct::convenience`** - 便捷 API（构建器、批量操作等）
- **`duct::async`** - 异步操作 API
- **`duct::raii`** - RAII 风格的资源管理
- **`duct::utils`** - 工具类（计时器、作用域守卫、字符串构建器等）
- **`duct`** - 日志和指标（`LogLevel`、`Logger`、`Counter`、`Gauge`、`Histogram`）

### 日志接口

```cpp
// 设置日志级别
set_log_level(LogLevel::kDebug);

// 全局日志函数
info("信息消息");
warning("警告消息");
error("错误消息");

// 使用宏
DUCT_INFO("使用宏记录信息");

// 流式日志
DUCT_LOG_STREAM(kInfo) << "值: " << 42;

// 自定义日志记录器
auto logger = std::make_shared<PrefixLogger>(
    std::make_shared<ConsoleLogger>(),
    "[MyApp] ");
set_logger(logger);
```

### 指标收集

```cpp
auto& registry = MetricRegistry::instance();

// 计数器
auto counter = registry.get_counter("requests_total");
counter->increment();
counter->increment(5);

// 仪表
auto gauge = registry.get_gauge("active_connections");
gauge->set(10);
gauge->increment();  // 11
gauge->decrement();  // 10

// 直方图
auto histogram = registry.get_histogram("request_duration_ms");
histogram->observe(10.5);
histogram->observe(20.3);
std::cout << "平均值: " << histogram->mean() << std::endl;
```

## 错误处理

### Status/Result 模式（推荐）

```cpp
auto result = dial("tcp://127.0.0.1:9000");
if (!result.ok()) {
  std::cerr << "错误: " << result.status().to_string() << std::endl;
}
```

### 异常模式

```cpp
try {
  auto pipe = dial("tcp://127.0.0.1:9000").value_or_throw();
  pipe->send(msg, {}).status().throw_if_error();
} catch (const duct::Exception& e) {
  std::cerr << "异常: " << e.what() << std::endl;
}
```

## 设计原则

本库严格遵循以下软件工程原则：

- **KISS** - 保持简单直观的 API 设计
- **DRY** - 统一的抽象，避免重复代码
- **SOLID** - 清晰的职责分离和可扩展设计
- **YAGNI** - 只实现当前需要的功能

## 项目结构

```
include/duct/
├── duct.h              # 核心 API
├── message.h           # 消息类型
├── status.h            # 错误处理
├── address.h           # 地址解析
├── convenience.h       # 便捷 API
├── async.h             # 异步 API
├── raii.h              # RAII 包装器
├── utils.h             # 工具类
├── logging.h           # 日志和指标
└── ...
```

## 项目状态

当前版本：**0.1.0**（开发中）

查看 `TODO.md` 了解计划的功能和里程碑。

## 许可证

待定

## 贡献

欢迎提交 Issue 和 Pull Request！
