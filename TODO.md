# duct TODO / Roadmap

This document captures the initial design decisions and the planned feature set.

## Scope
- Message-oriented IPC + networking for distributed game server processes
- Local fast path: `flatProto` + shared memory payload + notification wakeups
- Cross-machine support: TCP (optional TLS/mTLS)

## Milestones

### M0: Project skeleton (now)
- CMake build, headers, minimal TCP framing transport
- Example binaries to validate compilation and basic send/recv

### M1: Core API + options
- Public API: `listen`/`dial`, `Pipe`, `Message`, `SendOptions`/`RecvOptions`
- Address parsing and scheme dispatch (`shm/uds/pipe/tcp`)
- Timeouts, close semantics, cancellation hooks (if needed)

### M2: QoS + backpressure (per-connection)
- Queue limits: `snd_hwm_bytes|msgs`, `rcv_hwm_bytes|msgs`
- Backpressure policy (on HWM):
  - `block` (default)
  - `drop_new`
  - `drop_old`
  - `fail_fast` (`EAGAIN`)
- TTL/deadline per message
- Priority scheduling:
  - Reserve `channel_id` in the protocol
  - Start with a single default channel; later add multi-queue WRR/DRR
- Rate limiting (token bucket), per pipe and/or per socket

### M3: at-least-once reliability (per-connection)
- Handshake with `session_id` (changes on reconnect)
- Sequence numbers:
  - `seq` (send)
  - `ack` (cumulative ack; optional SACK bitmap later)
- Retransmit:
  - RTO timer + bounded retry policy
  - Inflight limits bound memory and induce backpressure
- Deduplication:
  - Receiver sliding window for `(session_id, seq)` to prevent duplicate delivery
- NOTE: at-least-once implies duplicates can happen; recommend upper layer idempotency

### M4: Transports
- `uds://` (Unix domain socket) with same framing/protocol
- `pipe://` (Windows named pipe) with same framing/protocol
- `shm://`:
  - Bootstrap/rendezvous: local `uds` socket for exchanging a connection id (initial impl)
  - Per-pipe TX/RX rings storing small descriptors (not raw payload)
  - Payload stored in shared memory pool/slab for true zero-copy
  - Notification:
    - Linux: `eventfd` (or futex)
    - Windows: Event object
    - macOS: kqueue EVFILT_USER or semaphore-based
  - Crash resilience + cleanup strategy for orphaned shm segments

### M5: Linux io_uring backend
- TCP/UDS send/recv via io_uring
- Optional busy-spin (spin-then-park) for ultra-low latency (off by default)
- Fallback to epoll if io_uring not available

### M6: Security (optional)
- TLS/mTLS for `tcp://` (configurable)
- Authentication/authorization hooks (if needed)

## Protocol (initial plan)
- Message framing with a fixed header (network byte order) and payload
- Reserve fields for:
  - `channel_id` (future multiplexing)
  - `session_id`, `seq`, `ack` (reliability)
  - `frag` fields for fragmentation/reassembly (payload > 64KB)
- Constraints:
  - Default max frame payload: 64KB
  - Larger messages: fragmentation (opt-in, bounded, with reassembly timeouts)
