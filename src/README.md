# Redis source and integration

This directory contains the Redis server, its data structures, client tools, and the integration with PMEM and PPF. It includes substantial upstream Redis code. Wookyung Lee's OMA contribution is in the [bundled allocator](../deps/memkind/OMA.md).

## Reading guide

| File | What to inspect |
| --- | --- |
| [server.c](server.c), [server.h](server.h) | Server initialization, shared state, PMEM setup, and BPF setup paths |
| [networking.c](networking.c) | Client request handling and consumption of packet metadata from BPF maps |
| [t_string.c](t_string.c) | String commands, including SET/GET and conditional BPF paths |
| [sds.c](sds.c) | Dynamic strings, including `sdslenbpf` for the BPF-enabled path |
| [zmalloc.c](zmalloc.c), [nvm.c](nvm.c) | Memory allocation wrappers and NVM support |
| [ae.c](ae.c), [anet.c](anet.c) | Event loop and socket helpers |
| [Makefile](Makefile) | `USE_NVM` and `USE_BPF` options and linked libraries |

For the PPF path, read `networking.c` with the [kernel-side program](../BasicBPF/packet_size_kern.c). For OMA, follow the NVM library selection in `Makefile` to `deps/memkind/jemalloc/src/arena.c`.

Build from the repository root using the [top-level instructions](../README.md#build-and-run). The NVM build requires the PMEM mount and configuration described there. OMA is controlled by the bundled allocator's `WKLEE` path; `USE_BPF` alone is not an OMA toggle.
