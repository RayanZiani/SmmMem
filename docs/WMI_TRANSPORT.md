# WMI Transport — Phase 1

## Scope

The current `src/` client uses the Windows `Advapi32.dll` WMI block API:

1. `WmiOpenBlock` opens the project-owned ACPI/WMI data block.
2. `WmiExecuteMethodW` invokes method `1` with a fixed request buffer.
3. The ACPI method copies the request to the mailbox and signals the configured SMI.
4. The response is returned through the WMI method output buffer.
5. `WmiCloseBlock` closes the block.

The mapper has the same transport shape but uses a separate GUID, instance list,
request format, and command namespace. The two protocols must remain separately
identified in documentation and test output.

## Current identifiers

| Transport | GUID | Method | Primary instances | Request size |
|---|---|---:|---|---:|
| `src/` memory API | `A0C9F8DE-0B71-42A8-B967-E538EACB6F21` | 1 | `Mem_0`, `SMMM_0`, `0_0` | 4096 |
| `mapper/` control API | `9B6F1A20-31D5-44DF-9A9C-157F4307914B` | 1 | `SmmMapper_0`, `SMMP_0`, `0_0` | 4096 |

Identifiers are project-owned and must not be changed during a benchmark. Any
future transport experiment needs a new experiment identifier and a separate
result set so that measurements remain attributable.

## Phase 1 decisions

- The first benchmark uses `CMD_PING` only.
- It does not read or write arbitrary memory.
- It does not stage, reload, or unload a payload.
- Request pacing is explicit and deterministic; the default is one request at a time.
- Each result records the selected instance, iteration, elapsed time, and status.
- A failed or malformed response stops the run and is reported rather than retried indefinitely.

## COM comparison

The existing direct block API is retained as the reference implementation. A
future COM implementation may be added as a separately selectable transport,
but it must preserve the same command semantics, bounds, error handling, and
result schema. It must not silently change the provider or identifier used by
the reference path.

## Read-only inventory

`tools/WmiInventory.exe` uses `IWbemLocator` and `IWbemServices::ExecQuery` to
enumerate class names exposed in `ROOT\WMI`. It is intentionally independent of
the SmmMem GUID and does not invoke a method, write a mailbox, or trigger an
SMI. Save its CSV output as the pre-installation or post-installation baseline
for a lab experiment:

```bat
tools\Work\WmiInventory.exe > wmi-root-wmi.csv
```

The inventory is a snapshot, not a security verdict. Compare two snapshots
using a sorted diff and record the Windows build and installed hardware for
each capture.

`tools/WmiProviderInventory.exe` provides a separate read-only provider
snapshot from `ROOT\CIMV2` using the `__Win32Provider` class. It records the
provider name, CLSID, and hosting model without invoking provider methods:

```bat
tools\Work\WmiProviderInventory.exe > wmi-providers.csv
```

Provider names and CLSIDs are inventory data, not identifiers to copy into the
project. Any comparison must preserve the original values and record the
machine and Windows build from which the snapshot was taken.

## Phase 1 exit criteria

Completed:

- current project-owned WMI identifiers and mailbox boundaries are documented;
- class and provider inventories are available as read-only CSV tools;
- snapshots can be compared with schema validation and deterministic exit codes;
- the reference transport has a ping-only latency benchmark.

Deferred:

- a COM `ExecMethod` path for the privileged project protocol;
- transport equivalence benchmarking between direct WMI and COM;
- batching memory operations;
- changing mailbox sizes or protocol fragmentation.

These items require a separate compatibility and authorization design. The
current phase deliberately avoids changing the privileged protocol or adding
more memory operations to the benchmark workload.

Use the repository comparison tool to produce both a summary and a complete
row-by-row report. It supports both the class snapshot schema
`namespace,class_name` and the provider snapshot schema
`name,clsid,hosting_model`:

```bat
tools\Work\WmiInventory.exe > before.csv
rem Perform the authorized lab change or installation here.
tools\Work\WmiInventory.exe > after.csv
py tools\compare_wmi_snapshots.py before.csv after.csv --output wmi-diff.csv --fail-on-change
```

For provider snapshots:

```bat
tools\Work\WmiProviderInventory.exe > providers-before.csv
rem Perform the authorized lab change or installation here.
tools\Work\WmiProviderInventory.exe > providers-after.csv
py tools\compare_wmi_snapshots.py providers-before.csv providers-after.csv --output providers-diff.csv --fail-on-change
```

Exit codes are `0` for no change, `1` for a detected change when
`--fail-on-change` is used, and `2` for invalid input or an I/O error.
