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
