# duct

`duct` is a C++ message-oriented networking/IPC library for low-latency server-to-server communication.

Design goals (high level)
- Prefer fastest local IPC first: `flatProto`-friendly zero-copy + shared memory + notification
- Fallback transports: Unix domain socket (Windows supported too) -> named pipe -> `127.0.0.1` TCP -> TCP
- Built-in QoS/backpressure (default: block) and optional `at-least-once` reliability
- Linux: optional `io_uring` backend; other platforms: epoll/kqueue/IOCP style backends

Status
- This repo is currently a skeleton. See `TODO.md` for the planned feature set and milestones.

Address schemes (planned)
- `shm://<name>` shared memory transport (local)
- `uds://<path>` Unix domain socket (local)
- `pipe://<name>` Windows named pipe (local)
- `tcp://<host>:<port>` TCP (local/remote)

Build (current skeleton)
```bash
cmake -S . -B build
cmake --build build
```

Run example
```bash
./build/duct_echo_server tcp://127.0.0.1:9000
./build/duct_echo_client tcp://127.0.0.1:9000 "hello"

# shared memory (local)
./build/duct_echo_server shm://gamebus
./build/duct_echo_client shm://gamebus "hello"
```
