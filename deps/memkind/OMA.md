# OMA implementation guide

Wookyung Lee's contribution to the collaborative X-redis project is OMA (Opportune Memory Allocator). OMA moves chunk allocation and initialization into a background thread to reduce allocation stalls in the request path.

## Where to start

1. Read the [OMA design and evaluation](../../README.md#oma-design) for the problem and paper results.
2. Open [jemalloc/src/arena.c](jemalloc/src/arena.c) and locate the `WKLEE` blocks.
3. Follow `arena_run_split_remove` to the condition-variable notification, then `arena_chunk_alloc_background_thread` to the actual allocation.
4. Inspect [src/memkind_pmem.c](src/memkind_pmem.c) for the underlying PMEM file allocation path.
5. Check [build-memkind.sh](../build-memkind.sh) and the Redis [Makefile](../../src/Makefile) to see which allocator library is built and linked.

## Main symbols

| Symbol in `jemalloc/src/arena.c` | Role |
| --- | --- |
| `threadInit` | Initializes synchronization objects and creates the worker thread |
| `arena_run_split_remove` | Checks the preallocation condition while servicing an allocation |
| `arena_chunk_alloc_background_thread` | Waits for a notification and allocates a chunk under the arena lock |
| `global_arena` | Arena selected for background allocation |
| `next_chunk` | Chunk pointer produced by background allocation |
| `shared_v`, `cond_a`, `mutex_a` | Notification state and synchronization objects |
| `bg_alloc` | State used by the preallocation trigger |

## Policy in this checkout

The trigger checks the arena, background-allocation state, and target chunk, then compares the current run's `total_pages` with half the usable page count of a chunk. This is a fixed threshold. The paper describes an adaptive policy based on allocation duration, consumption, and remaining space in the latest chunk. This checkout's equivalence to the final evaluation version has not been established.

The allocator worker and trigger are useful entry points for review, but their presence alone does not demonstrate concurrency correctness or reproduce the paper's results. No new allocator test or benchmark result is claimed by this guide.

## Upstream context

This directory also contains upstream memkind documentation, examples, tests, and license notices. OMA is a modification within the bundled jemalloc, not a replacement for all of memkind. The separate [deps/jemalloc](../jemalloc/) copy is not the OMA implementation described here.
