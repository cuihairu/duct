#include "duct/duct.h"

#include <iostream>
#include <string>
#include <thread>

int main(int argc, char** argv) {
  if (argc != 3) {
    std::cerr << "usage: " << argv[0] << " <mode> <bus_name>\n";
    std::cerr << "modes: server, client\n";
    return 2;
  }

  std::string mode = argv[1];
  std::string bus_name = argv[2];

  if (mode == "server") {
    std::cout << "Starting Windows shared memory server on bus: " << bus_name << "\n";
    auto lis = duct::listen("shm://" + bus_name);
    if (!lis.ok()) {
      std::cerr << "listen failed: " << lis.status().message() << "\n";
      return 1;
    }

    std::cout << "Waiting for client connection...\n";
    auto pipe = lis.value()->accept();
    if (!pipe.ok()) {
      std::cerr << "accept failed: " << pipe.status().message() << "\n";
      return 1;
    }

    std::cout << "Client connected!\n";

    // Echo loop
    for (int i = 0; i < 5; ++i) {
      auto msg = pipe.value()->recv({});
      if (!msg.ok()) {
        std::cerr << "recv failed: " << msg.status().message() << "\n";
        break;
      }

      std::string received(msg.value().data(), msg.value().data() + msg.value().size());
      std::cout << "Server received: " << received << "\n";

      // Echo back
      auto st = pipe.value()->send(msg.value(), {});
      if (!st.ok()) {
        std::cerr << "send failed: " << st.status().message() << "\n";
        break;
      }
    }

    std::cout << "Server done.\n";
  } else if (mode == "client") {
    std::cout << "Starting Windows shared memory client to bus: " << bus_name << "\n";

    // Give server time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    auto pipe = duct::dial("shm://" + bus_name);
    if (!pipe.ok()) {
      std::cerr << "dial failed: " << pipe.status().message() << "\n";
      return 1;
    }

    std::cout << "Connected to server!\n";

    // Send messages
    for (int i = 0; i < 5; ++i) {
      std::string msg_str = "Message " + std::to_string(i) + " from client";
      auto msg = duct::Message::from_string(msg_str);

      auto st = pipe.value()->send(msg, {});
      if (!st.ok()) {
        std::cerr << "send failed: " << st.status().message() << "\n";
        break;
      }
      std::cout << "Client sent: " << msg_str << "\n";
    }

    // Receive echoes
    for (int i = 0; i < 5; ++i) {
      auto echoed = pipe.value()->recv({});
      if (!echoed.ok()) {
        std::cerr << "recv failed: " << echoed.status().message() << "\n";
        break;
      }

      std::string received(echoed.value().data(), echoed.value().data() + echoed.value().size());
      std::cout << "Client received echo: " << received << "\n";
    }

    std::cout << "Client done.\n";
  } else {
    std::cerr << "unknown mode: " << mode << "\n";
    return 2;
  }

  return 0;
}