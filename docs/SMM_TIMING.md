# SMM Timing and Request Budgets

## Scope

The timing instrumentation is enabled only in `src_dbg01`. It observes the
handler with `RDTSC` at entry and exit, stores cycle counts in the debug UEFI
variable, and does not write or adjust TSC, APIC timers, MSRs, or any other
time source.

## Recorded values

`SmmMemSmmDebug` records:

- total request count;
- aggregate error count;
- last command and status;
- last, maximum, and cumulative handler cycles;
- invalid magic, invalid size, invalid command, and sequence-anomaly counts;
- last observed request sequence.

The values are diagnostic counters, not a calibrated time unit. Converting
cycles to microseconds requires a separately recorded platform frequency and
must account for firmware and CPU power-management behavior.

The debug snapshot is persisted once every 64 handled requests, not on every
SMI, to avoid repeated non-volatile variable writes. A system that stops before
the next checkpoint may therefore expose a snapshot up to 63 requests old.

## Request budget

The debug build uses a fixed diagnostic budget of `5,000,000` TSC cycles per
request. Long virtual-memory walks and bounded list scans check this budget and
return `EFI_TIMEOUT` when it is exceeded. The budget is deliberately a safety
bound, not a claim about an acceptable production latency. It must be evaluated
on each target platform before changing it.

The release tree is not changed by this instrumentation. A timeout is reported
as an error and does not attempt to continue a partially completed operation.

The debug instrumentation does not move translation, process discovery, or
symbol resolution out of SMM. That architectural separation remains open and
requires a separate protocol design; the current implementation instead
enforces a bounded diagnostic budget and returns a failure status.

## Cache policy

The debug tree uses a one-entry process metadata cache keyed by PID. A cached
entry is reused only after the EPROCESS data and PID are read again; a failed
validation clears the entry. This limits stale-data exposure and avoids a
global process-list cache. No physical address or arbitrary user buffer is
cached.

## Platform limitations

SMM entry affects platform-wide execution, and this repository does not claim
that a software SMI is local to one CPU. Direct-dispatch behavior is platform
specific and remains a documentation/evaluation item. The safe fallback is the
standard registered software-SMI path.
