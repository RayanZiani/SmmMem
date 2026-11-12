# SmmMem Roadmap

Engineering goals, ordered by pillar. Every goal has a measurable acceptance
criterion — an item is only done when its criterion is reproducible on the
target board. Constraints that shape every decision:

- response data field is **352 bytes** per request (`RESPONSE_DATA_SIZE`);
- commands `1..11` are allocated; new commands start at `12`;
- SW SMI `0xD6` / WMI GUID `A0C9F8DE-…` identify the memory tree, `0xD5` /
  `9B6F1A20-…` the mapper;
- any protocol change must keep the trees independently flashable and
  rollback-safe (old client against new firmware must fail cleanly, never
  crash).

---

## Pillar 1 — Stealth / undetectability

Goal: nothing observable that distinguishes SmmMem traffic from ordinary WMI
use, and nothing measurable that distinguishes SMM entry/exit from baseline.

### S1. Batch command (`CMD_READ_BATCH`, id 12)
One WMI call carries N (address, size≤8) read requests; the response packs up
to 352 bytes of results. Kills the "one SMI per read" behavioral signature:
32 reads become 1 call.
- **Accept:** `WmiPingBench`-style measurement shows ≥90% fewer SMIs per
  memory operation vs the per-read baseline at identical throughput.

### S2. Client pacing and jitter
Randomized inter-call delay (configurable min/max window), no fixed periods,
burst limiting for large transfers.
- **Accept:** call interval distribution passes a periodicity check
  (χ²/kolmogorov) against a legitimate monitoring workload on the same box.

### S3. SMM handler time reduction
Port the debug build's request-cycle budget (`EFI_TIMEOUT`) into the release
tree; expand the 1-entry process cache to an N-entry LRU with CR3 revalidation;
keep PE/list walks bounded.
- **Accept:** 95th-percentile SMM handler duration ≤ 2× the median of a
  workload-free baseline; no request ever exceeds the hard budget.

### S4. Timer side-channel containment *(advanced)*
Evaluate TSC/APIC-timer correction around the handler (read on entry, restore
offset on exit) per the technique notes in `todo.md`.
- **Accept:** RDTSC-measured delta around SMI entry/exit is statistically
  indistinguishable from no-op SMIs on the same hardware — or the technique is
  documented as not viable and dropped.

### S5. Identifier and buffer mimicry *(long term)*
Mimic vendor-shaped GUID/UID patterns and legit buffer sizes; fragment staging
transfers to look like sensor/statistics reads.
- **Accept:** the installed ACPI/WMI surface is classified "plausible vendor
  device" by a reviewer given only the running OS; no artifact contains the
  literal markers `SMMM`/`MEMDEV`/`SmmMem`.

### S6. Silent release build
No debug UEFI variables, no serial output, no config-table markers in the
release tree.
- **Accept:** zero SmmMem strings/variables recoverable from a flashed
  release image by string and GUID sweep.

---

## Pillar 2 — Reliability ("infaillible")

Goal: no firmware path can crash, hang, or corrupt the memory map, whatever
the client does. Failure must always end in a clean, reported error.

### R1. Fix `DxeBridge.c` memory-map mutation
The temporary `EFI_CONVENTIONAL_MEMORY` → `EFI_RESERVED_MEMORY_TYPE` swap of
the SMM Communication Region descriptor has no crash-safe restore.
- **Accept:** either the mutation is removed entirely, or an unwind path
  restores the descriptor on every exit (including error paths) and this is
  exercised by a test that faults payload delivery mid-flight.

### R2. Global SMI watchdog
Hard cycle budget per request; on overrun: abandon the operation, clear the
mailbox, return `EFI_ABORTED`.
- **Accept:** a deliberately long request (huge translate list) returns
  `EFI_TIMEOUT`/`EFI_ABORTED` and the system continues serving requests.

### R3. Concurrent and malformed callers
Define behavior for two user-mode processes racing on the mailbox: serialize
with a generation/sequence token, reject stale requests.
- **Accept:** a fuzz harness (two processes, malformed buffers, truncated
  chunks, oversized fields) runs for 24h with zero hangs, zero reboots, and
  every malformed input produces a defined status code.

### R4. Power-state correctness
Mailbox and cache state across S3/S4/hibernate/fast startup: revalidate magic,
generation, and physical addresses after resume.
- **Accept:** sleep/resume and hibernate cycles with active traffic show no
  stale-pointer access (validated by the watchdog + sequence counters).

### R5. Mapper lifecycle integrity
Bounded tracked slots must degrade gracefully (documented or dynamic); unload
must provably unregister every handler and free every pool.
- **Accept:** load/unload loop of 1000 random payloads (valid + malformed)
  ends with zero leaked handlers, zero leaked pools, verified by instrumentation.

---

## Pillar 3 — Performance

Goal: memory operations fast enough for real workloads, bounded variance.

### P1. Batch throughput
With S1 in place: measure per-byte read cost of batched vs single reads.
- **Accept:** batched reads deliver ≥5× the bytes/ms of single reads for
  4KB working sets.

### P2. Latency distribution
Continuously benchmark with `tools/WmiPingBench.c` + `tools/summarize_ping.py`
(schema `smmmem.wmi-ping.v1`).
- **Accept:** per-command latency table (median, p95, p99) exists for every
  command, refreshed on every protocol change, stored as reproducible CSV.

### P3. QWORD fast paths everywhere
Audit that all request parsing, copying, and comparison in the SMM handler use
the QWORD copy/zero routines with no byte loops on hot paths.
- **Accept:** code audit clean + no regression in P2 numbers after the audit.

---

## Execution order

1. **S1** (`CMD_READ_BATCH`) — highest leverage: stealth + performance at once.
   Touches `src/Common.h` (command id 12, batch request/response structs),
   `src/Smm.c` (handler), `src/Client.c` + `src/Api.h` (client + public API).
2. **S2** (pacing/jitter) — pure client-side, immediate.
3. **R1 + R2** (watchdog + mutation fix) — closes the two known correctness
   holes before anything else is built on top.
4. **S3** (cache + budget port) — needs the R2 budget as its base.
5. **R3** (fuzz harness) — once the surface stops moving.
6. **S4/S5/S6, R4/R5, P1–P3** — iterate.

## Appendix — `CMD_READ_BATCH` design (task 1 spec)

Constraints from `src/Common.h`: request buffer 4096 B, response data 352 B.
Entries are fixed 8-byte virtual reads inside one process, so the response
cost per entry is exactly 8 B.

```c
#define CMD_READ_BATCH     12U
#define BATCH_ENTRY_SIZE    8U   /* fixed QWORD read per entry            */
#define BATCH_MAX_ENTRIES  43U   /* 352 - 8 (response header) = 344; 344/8 */

/* Request: one process per batch (same CR3). */
typedef struct {
    UINT32 Magic;        /* BATCH_MAGIC                       */
    UINT32 Count;        /* 1..BATCH_MAX_ENTRIES              */
    UINT64 VirtualAddresses[BATCH_MAX_ENTRIES];
} BATCH_REQUEST;

/* Response: partial results up to the first failed entry are valid. */
typedef struct {
    UINT32 Status;       /* EFI_SUCCESS or the failing status */
    UINT32 DoneCount;    /* entries completed before failure  */
    UINT64 Values[BATCH_MAX_ENTRIES]; /* only [0..DoneCount) valid */
} BATCH_RESPONSE;
```

Rules:

- Reuse the existing `ReadVirt` internal path per entry (handles page-boundary
  splits), so batch reads inherit its validation.
- First failing entry stops the batch: return its status + `DoneCount`; no
  silent skips.
- `Count` outside `1..BATCH_MAX_ENTRIES` or bad magic → status error, no
  memory access. Zero host-side work beyond the mailbox validation.
- Client side: `ReadVirtBatch(pid, *addresses, count, *out)` in `Api.h`;
  `Client.c` chunks batches into the 4096-byte request buffer and reports the
  per-batch SMI count to callers for benchmarking.

## Deferred / optional

- Intel platform portability check.
- `docs/CHANGELOG.md`, semantic versioning for firmware and client API.
- COM `IWbemServices::ExecMethod` transport alternative (only if the
  Advapi32 path ever becomes a liability).
