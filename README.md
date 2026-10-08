# Open-Loop TCP Load Generator

A TCP load generator that sends scheduled requests and measures the latency from when each request was due.

Closed-loop benchmarking under reported p99 latency by more than 8,000x against a server with injected stalls.

## The Problem

Most generators are closed-loop, they send a request, wait for the response, then send the next. Therefore the server controls the sending rate, and when the server stalls, the client stops.

If a server was to freeze for 200 ms, a closed-loop client only sent out one request, and waits out the freeze, recording one sample. But a service taking 1,000 requests per-second would have tried sending 200 requests, and would've lost all of them.

The percentiles are computed over a sample that excludes the slow requests. This is strongest exactly when the server is running at its worst. This is coordinated omission, named by Gil Tene.

## How It Works

## Results

All runs: single connection, 6-byte messages, loopback, macOS. Closed-loop runs sent 10,000,000 requests (about 2 to 2.5 minutes each), while open-loop runs sent 60,000 requests (60 seconds each). Stalls followed the same schedule in every run, so both modes spent the same share of time against a frozen server.

### Healthy server

| mode | p50 | p99 | p99.9 | p99.99 | p99.999 | max | throughput |
|---|---|---|---|---|---|---|---|
| closed | 12µs | 16µs | 22µs | 49µs | 104µs | 13.4ms | 81,302 req/s |
| open (1k/s) | 264µs | 308µs | 1.7ms | 11.9ms | 14.6ms | 14.6ms | 1,000 req/s |

### Server stalling 200ms every 1s

| mode | p50 | p99 | p99.9 | p99.99 | p99.999 | max | throughput |
|---|---|---|---|---|---|---|---|
| closed | 12µs | **22µs** | 55µs | 92µs | 205ms | 210ms | 64,886 req/s |
| open (1k/s) | 265µs | **193ms** | 203ms | 204ms | 204ms | 204ms | 1,000 req/s |

The server was unresponsive for about 20% of each stalled run. Closed-loop reports a p99 of 22µs, six microseconds above its own health baseline of 16µs. Open-loop reports 193ms at the same percentile, a factor of abour 8,800.

Closed-loop doesn't ignore the stall entirely, it only shows at p99.999 (205ms), one smaple in 100,000. Under that, every other reported percentile looks healthy, since the requests that would have been slow were not sent.

Throughput shows the same blind spot, but from the other side. The stalling server deliverd 64,886 req/s against 81,302 for the healthy one. This was a 20% drop that matches the 20% of clock time spent frozen. The latency percentiles report almost no change aswell.

## Usage

The project builds two binaries. `echo_server` is the target under test, `loadgen` is the measurement tool.

### echo_server

A TCP server that reads bytes and writes them back. It can be told to freeze on a timer, which simulates a garbage-collection pause, a lock, or a blocking disk write.

```
./echo_server [--port N] [--stall-ms N] [--stall-every-sec N]
```

| flag | default | meaning |
|---|---|---|
| `--port` | 8080 | port to listen on |
| `--stall-ms` | 0 | how long to freeze, in milliseconds |
| `--stall-every-sec` | 0 | how often to freeze, in seconds |

Both stall flags must be set for stalling to happen, else it is ignored.

### loadgen

```
./loadgen COUNT MESSAGE [--mode open|closed] [--rate N]
                        [--stall-ms N] [--stall-every-sec N]
```
| argument | default | meaning |
|---|---|---|
| `COUNT` | required | number of requests to send |
| `MESSAGE` | required | payload |
| `--mode` | closed | `open` schedules requests in advance, `closed` waits for each reply |
| `--rate` | — | requests per second (required in open mode) |
| `--stall-ms` | 0 | recorded in the CSV only |
| `--stall-every-sec` | 0 | recorded in the CSV only |

The two stall flags do not make `loadgen` do anything. They are passed so the server's configuration is recorded with the measurements, the latency number is meaningless without the conditions that produce it.

### run_benchmark.sh

Starts the server, runs the load generator against it, and kills the server afterwards even if the run fails.

```
./run_benchmark.sh COUNT MSG [STALL_MS] [STALL_EVERY] [MODE] [RATE]
```

### Examples

Closed loop against a healthy server:

```
$ ./run_benchmark.sh 10000000 Hello
Average: 11 microseconds
Lowest: 7 microseconds
Highest: 13419 microseconds
Median: 12 microseconds

99th Percentile: 16 microseconds
99.9th Percentile: 22 microseconds
99.99th Percentile: 49 microseconds
99.999th Percentile: 104 microseconds
```

Open loop at 1,000 req/s against a server that freezes 200ms every second:

```
$ ./run_benchmark.sh 60000 Hello 200 1 open 1000
Average: 18305 microseconds
Lowest: 32 microseconds
Highest: 204445 microseconds
Median: 265 microseconds

99th Percentile: 192611 microseconds
99.9th Percentile: 203376 microseconds
99.99th Percentile: 204396 microseconds
99.999th Percentile: 204445 microseconds
```
Every run appends a row to `results/results.csv`.

## Building

No dependencies other than a C++17 compiler and the POSIX socket headers.

```bash
clang++ -std=c++17 -Wall -Wextra -Wshadow -O2 -g -o echo_server src/echo_server.cpp
clang++ -std=c++17 -Wall -Wextra -Wshadow -O2 -g -o loadgen     src/loadgen.cpp
```
`g++` works the same way. Developed and measured on macOS (Apple Silicon).
`-O2` matters: the measurement loop should be compiled the way real code is.
`-g` is kept so the binaries stay profileable.

The results directory must exist before the first run:

```bash
mkdir -p results
```

## Limitations

These bound what the numbers mean. Read them before quoting any figure above.

- **Single connection, single thread.** One client socket and one request at a time. Real servers face many concurrent connections, and contention between them is not measured here.

- **Open-loop latency includes the generator's own scheduling error.** On a
  healthy server at 1,000 req/s the open-loop p99 is 308µs against closed
  loop's 16µs. That gap is the client waking late from `sleep_until`, which
  macOS delays at millisecond intervals for power management.

- **Percentiles need samples.** A trustworthy p99.9 needs at least 10,000
  measurements, p99.99 needs 100,000, and so on. Runs shorter than that
  report p99.99 and above as equal to the maximum.

- **Loopback only.** Client and server run on the same machine, so there is
  no network between them. These are not internet latencies, and the absolute numbers would look
  very different across a real link.

- **`errors` must be zero for a run to be comparable.** Failed requests are
  counted but contribute no latency sample.

- **Nagle's algorithm is not disabled.** `TCP_NODELAY` is not set, so small
  writes may be delayed by the kernel.

## CSV Output

Every run appends one row to `results/results.csv`, creating the header if the
file is empty.

| column | meaning |
|---|---|
| `timestamp` | local time the run finished |
| `machine` | hostname |
| `os` | operating system the run was made on |
| `count` | requests measured |
| `msg_bytes` | payload size, including the appended newline |
| `mode` | `open` or `closed` |
| `target_rate` | requests per second requested (empty in closed mode) |
| `stall_ms` | length of each injected server stall |
| `stall_every` | seconds between injected stalls |
| `duration_s` | clock time for the measured loop |
| `throughput_rps` | `count / duration_s` |
| `errors` | requests that failed or returned short |
| `min_us` | fastest request |
| `p50_us` … `p99999_us` | percentiles, nearest-rank |
| `max_us` | slowest request |
| `avg_us` | mean |

All latencies are in microseconds.

## Roadmap

- [x] Closed-loop mode with per-request timing
- [x] Open-loop scheduling with due-time latency
- [x] Configurable stall injection in the test server
- [x] CSV output with run conditions recorded
- [ ] Record scheduler lateness separately from server latency
- [ ] `--out` flag for raw per-request latency dumps
- [ ] Latency histogram and percentile curve charts
- [ ] Multiple concurrent connections
- [ ] `--duration` flag instead of a fixed request count