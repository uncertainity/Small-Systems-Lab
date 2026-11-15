# Systems Lab

A collection of small C++17 programs for practicing systems programming
concepts: memory management, Linux file I/O, network sockets, and transformer
inference.

## Requirements

- Linux or another POSIX-compatible environment
- A C++17 compiler such as GCC or Clang

Each example is standalone and can be compiled directly with `g++`. Generated
executables use the `.exe` suffix so they are covered by this directory's
`.gitignore`.

## Project layout

| Directory | Topic | Programs |
| --- | --- | --- |
| `Memory/` | Manual memory management | Bump arena allocator and small-buffer optimization |
| `Linux_IO/` | File-backed block storage | Virtual disk using `pread` and `pwrite` |
| `Networks/` | Linux socket programming | TCP and UDP echo client/server pairs |
| `Inference/` | Transformer inference | Character-level attention and greedy generation |

## Memory

### Bump arena allocator

`arena_alloc.cpp` implements a fixed-capacity arena that supports aligned
object construction, reverse-order destruction, move construction, and move
assignment. Its `main` function contains assertion-based tests for allocation,
alignment, overflow handling, destructor order, moves, and exceptions.

```bash
cd Memory
g++ -std=c++17 -Wall -Wextra -Wpedantic arena_alloc.cpp -o arena.exe
./arena.exe
```

### Small-buffer optimization

`buffer_opt.cpp` stores buffers of up to 16 bytes inline and allocates larger
buffers on the heap. Its assertion-based tests exercise construction, copying,
moving, assignment transitions, self-assignment, and boundary sizes.

```bash
cd Memory
g++ -std=c++17 -Wall -Wextra -Wpedantic buffer_opt.cpp -o buffer.exe
./buffer.exe
```

## Linux I/O

`virtual_disk.cpp` creates a 512 MiB file-backed virtual disk with 4 KiB
blocks. It writes random data to 100 block locations, reopens the disk, and
checks the persisted bytes using robust `pread` and `pwrite` loops.

```bash
cd Linux_IO
g++ -std=c++17 -Wall -Wextra -Wpedantic virtual_disk.cpp -o virtual_disk.exe
./virtual_disk.exe
```

The program creates or replaces `disk.img` in the current directory. Ensure at
least 512 MiB of disk space is available before running it.

## Networks

The networking examples implement echo services on `127.0.0.1:9000`. Compile
the four programs first:

```bash
cd Networks
g++ -std=c++17 -Wall -Wextra -Wpedantic tcp_server.cpp -o tcp_server.exe
g++ -std=c++17 -Wall -Wextra -Wpedantic tcp_client.cpp -o tcp_client.exe
g++ -std=c++17 -Wall -Wextra -Wpedantic udp_server.cpp -o udp_server.exe
g++ -std=c++17 -Wall -Wextra -Wpedantic udp_client.cpp -o udp_client.exe
```

### TCP echo

Start the server in one terminal:

```bash
./tcp_server.exe
```

Then run the client from the same directory in another terminal:

```bash
./tcp_client.exe
```

The client sends `Hello from TCP client.` and prints the echoed response. The
server handles one connection and exits after the client disconnects.

### UDP echo

Start the server in one terminal:

```bash
./udp_server.exe
```

Then run the client in another terminal:

```bash
./udp_client.exe
```

The client sends one datagram and prints the echoed response. The UDP server
continues waiting for datagrams until it is stopped with `Ctrl+C`.

## Dummy transformer inference

`dummy_transformer.cpp` is an educational, untrained transformer-style
inference pass. It:

- tokenizes the lowercase letters `a` through `z`;
- creates random 5-dimensional embeddings and model weights;
- computes single-head scaled dot-product attention for the final token;
- projects the context vector to a 26-character probability distribution; and
- greedily appends five characters to the fixed prompt `aaabcde`.

```bash
cd Inference
g++ -std=c++17 -Wall -Wextra -Wpedantic dummy_transformer.cpp \
    -o dummy_transformer.exe
./dummy_transformer.exe
```

The final lines look similar to:

```text
Original String:aaabcde
Generated String:dddtt
```

The exact generated text is not meaningful: the weights are random and are
never trained.

### Inference flow

For sequence length `T`, model dimension `D = 5`, and vocabulary size
`V = 26`:

```text
embeddings (T x D)
    |-- Wk --> keys   (T x D)
    |-- Wv --> values (T x D)
last embedding -- Wq --> final query (D)
                         |
                         v
       softmax(query * keys^T / sqrt(D)) (T)
                         |
                         v
              weighted value context (D)
                         |
                       Wout
                         |
                         v
                 token probabilities (V)
```

Only lowercase input is supported. The example intentionally omits training,
positional encoding, feed-forward layers, normalization, residual connections,
batching, and a KV cache.

### Sanitizer check

An optional AddressSanitizer and UndefinedBehaviorSanitizer build can be used
for runtime checking:

```bash
g++ -std=c++17 -O1 -g -fsanitize=address,undefined \
    -fno-omit-frame-pointer dummy_transformer.cpp \
    -o dummy_transformer_san.exe
./dummy_transformer_san.exe
```

