# Redis utilities

This directory contains Redis administration, development, and diagnostic tools. It is supporting infrastructure rather than the main PPF or OMA implementation.

| Path | Purpose |
| --- | --- |
| [create-cluster/](create-cluster/) | Scripts for creating a local Redis Cluster |
| [install_server.sh](install_server.sh), [redis_init_script](redis_init_script) | Server installation and service startup helpers |
| [redis-copy.rb](redis-copy.rb) | Redis data-copy utility |
| [speed-regression.tcl](speed-regression.tcl) | Performance regression utility |
| [corrupt_rdb.c](corrupt_rdb.c) | RDB corruption utility for development/testing |
| [generate-command-help.rb](generate-command-help.rb) | Command-help generation |
| [lru/](lru/), [hashtable/](hashtable/), [hyperloglog/](hyperloglog/) | Data-structure experiments and diagnostics |
| [graphs/](graphs/) | Historical Redis graphing utilities |

The X-redis paper figures are in [docs/figures](../docs/figures/README.md), and their extraction script is in [docs/scripts](../docs/scripts/). For allocation changes, see the [OMA implementation guide](../deps/memkind/OMA.md).
