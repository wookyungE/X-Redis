# Test directory

This directory contains Redis tests and supporting scripts. The directory structure does not by itself establish dedicated coverage for OMA or PPF.

| Path | Purpose |
| --- | --- |
| [unit/](unit/) | Tests of Redis commands and individual behaviors |
| [integration/](integration/) | Tests spanning server subsystems |
| [cluster/](cluster/) | Redis Cluster tests |
| [sentinel/](sentinel/) | Redis Sentinel tests |
| [support/](support/), [helpers/](helpers/) | Shared test infrastructure |
| [assets/](assets/) | Test fixtures |
| [test_helper.tcl](test_helper.tcl), [instances.tcl](instances.tcl) | Tcl test harness and instance support |

The tree also includes `py-tests/`, `bkc_scripts/`, and `cce_scipts/` with additional scripts. Inspect their environment assumptions before use.

The [published evaluation](../README.md#evaluation) is separate from this test directory. A reproducible runner for the paper's OMA comparison has not been established in this checkout, and these tests were not executed as part of the documentation update.
