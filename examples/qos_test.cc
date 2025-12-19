#include "duct/duct.h"

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "usage: " << argv[0] << " <mode>\n";
    std::cerr << "modes: server, client-block, client-drop, client-failfast\n";
    return 2;
  }

  std::string mode = argv[1];

  if (mode == "server") {
    // Server: simply echo back messages
    auto lis = duct::listen("tcp://127.0.0.1:9001");
    if (!lis.ok()) {
      std::cerr << "listen failed: " << lis.status().message() << "\n";
      return 1;
    }

    std::cout << "Server listening on tcp://127.0.0.1:9001\n";
    auto pipe = lis.value()->accept();
    if (!pipe.ok()) {
      std::cerr << "accept failed: " << pipe.status().message() << "\n";
      return 1;
    }

    std::cout << "Client connected\n";
    for (int i = 0; i < 50; ++i) {
      auto msg = pipe.value()->recv({});
      if (!msg.ok()) {
        std::cerr << "recv failed: " << msg.status().message() << "\n";
        break;
      }
      std::cout << "Received message " << i << "\n";
      // Slow down the server to create backpressure
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      auto st = pipe.value()->send(msg.value(), {});
      if (!st.ok()) {
        std::cerr << "send failed: " << st.status().message() << "\n";
        break;
      }
    }
  } else {
    // Client modes with different QoS settings
    duct::DialOptions opts;
    opts.timeout = std::chrono::seconds(5);

    if (mode == "client-block") {
      opts.qos.snd_hwm_bytes = 1024;  // Small queue to test blocking
      opts.qos.backpressure = duct::BackpressurePolicy::kBlock;
      std::cout << "Client with BLOCK backpressure\n";
    } else if (mode == "client-drop") {
      opts.qos.snd_hwm_bytes = 1024;
      opts.qos.backpressure = duct::BackpressurePolicy::kDropNew;
      std::cout << "Client with DROP_NEW backpressure\n";
    } else if (mode == "client-failfast") {
      opts.qos.snd_hwm_bytes = 1024;
      opts.qos.backpressure = duct::BackpressurePolicy::kFailFast;
      std::cout << "Client with FAIL_FAST backpressure\n";
    } else {
      std::cerr << "unknown mode: " << mode << "\n";
      return 2;
    }

    auto pipe = duct::dial("tcp://127.0.0.1:9001", opts);
    if (!pipe.ok()) {
      std::cerr << "dial failed: " << pipe.status().message() << "\n";
      return 1;
    }

    // Send messages rapidly to test QoS
    for (int i = 0; i < 50; ++i) {
      std::string msg_str = "message " + std::to_string(i);
      auto msg = duct::Message::from_string(msg_str);
      auto st = pipe.value()->send(msg, {});
      if (!st.ok()) {
        std::cerr << "send " << i << " failed: " << st.status().message() << "\n";
        break;
      } else {
        std::cout << "Sent message " << i << "\n";
      }
    }

    // Receive echoes
    for (int i = 0; i < 50; ++i) {
      auto echoed = pipe.value()->recv({});
      if (!echoed.ok()) {
        std::cerr << "recv " << i << " failed: " << echoed.status().message() << "\n";
        break;
      }
      std::string received(echoed.value().data(), echoed.value().data() + echoed.value().size());
      std::cout << "Received echo: " << received << "\n";
    }
  }

  return 0;
}