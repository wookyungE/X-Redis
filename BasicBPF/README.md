# PPF packet inspection

This directory contains the eBPF packet inspection program used by PPF and standalone utilities used during development. It belongs to the collaborative PPF implementation; the personal OMA contribution is documented [separately](../deps/memkind/OMA.md).

| File or directory | Purpose |
| --- | --- |
| [packet_size_kern.c](packet_size_kern.c) | Kernel-side packet inspection, port mapping, and packet metadata queues |
| [packet_size_user.c](packet_size_user.c) | Standalone user-space BPF loading and map-access utility |
| [bpf_legacy.h](bpf_legacy.h) | BPF compatibility definitions used by the programs |
| [client.c](client.c) | Small socket client for development |
| [1_make_k.sh](1_make_k.sh) | Compile `packet_size_kern.c` with Clang's BPF target |
| [1_make_u.sh](1_make_u.sh) | Compile the standalone user-space utility with libbpf |
| `test/`, `bak/`, and files ending in `_bak` | Experimental and backup variants |

## Connection to Redis

The integrated Redis path is in [src/server.c](../src/server.c) and [src/networking.c](../src/networking.c). The top-level build procedure compiles `packet_size_kern.o` here and copies it to the repository root. The standalone user utility is separate from that server integration.

Use the [top-level build instructions](../README.md#build-and-run). `1_make_u.sh` contains an absolute libbpf path from the original environment and needs adjustment before use elsewhere. Rebuild object files and executables for the target environment rather than relying on the checked-in binaries.
