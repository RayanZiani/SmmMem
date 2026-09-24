# SmmMem — Initial Test Matrix

This matrix is a template for controlled, repeatable validation. An empty cell means that the result has not yet been measured; it is not a compatibility claim.

## Platform matrix

| ID | Board / firmware | CPU | Windows build | Secure Boot | VBS | Source tree | Recovery verified | Result |
|---|---|---|---|---|---|---|---|---|
| LAB-01 | ASUS TUF X870 / record exact version | AMD AM5 | record exact build | record | record | `src_dbg01` | yes/no | pending |
| LAB-02 | record | AMD AM4 or equivalent | record | record | record | `src_dbg01` | yes/no | pending |
| LAB-03 | record | Intel platform | record | record | record | `src_dbg01` | yes/no | pending |

## Functional baseline

Run in this order, using synthetic data only:

| Test | Purpose | Expected result | Evidence |
|---|---|---|---|
| PING-01 | Verify WMI-to-SMM path | Valid response with matching command/sequence | client output + serial log |
| MAILBOX-01 | Validate mailbox initialization | Magic, sizes, offsets, and bounds accepted | debug state |
| READ-01 | Bounded physical read of a lab buffer | Exact expected bytes | request/response record |
| READ-02 | Bounded virtual read of a synthetic process buffer | Exact expected bytes | request/response record |
| ERROR-01 | Invalid magic and oversized request | Rejected without memory access | status + trace |
| ERROR-02 | Sequence mismatch and truncated response | Rejected or reported deterministically | status + trace |
| RECOVERY-01 | Restore original firmware | System returns to baseline | firmware hash + boot record |

## Measurement baseline

For each successful test, collect at least 30 samples where practical:

- WMI round-trip duration;
- SMM handler duration from entry to exit;
- request and response sizes;
- command status and timeout count;
- serial/debug trace records;
- CPU and system impact observable from the lab instrumentation.

Do not modify TSC, APIC timers, MSRs, or other system time sources to obtain measurements.

## Compatibility claims to verify

- whether the stated Windows privilege requirements match the actual client and diagnostic paths;
- whether Secure Boot and firmware verification behave as described on each test board;
- whether the ACPI table and WMI path are accepted consistently across firmware versions;
- whether the dynamic Windows structure discovery remains correct across tested builds;
- whether mapper payload size, relocation, cleanup, and communication-region behavior are safe at each boundary;
- whether the stated Intel compatibility is demonstrated or remains only a design expectation.
