# SmmMem Research Scope

## Authorized use

SmmMem is a firmware-security research proof of concept. Use it only on
hardware owned by the operator or covered by explicit written authorization.
The test system must be isolated from production networks and sensitive
accounts, and experiments must use synthetic processes and non-sensitive
buffers by default.

Do not test on third-party systems, shared workstations, or systems containing
real credentials. Do not flash firmware until recovery has been verified on a
disposable setup.

## Required recovery preparation

Before every firmware experiment:

1. Record the board, CPU, firmware version, firmware configuration, and Windows
   build.
2. Save and hash the original firmware image when technically possible.
3. Verify the board-specific recovery process and recovery media.
4. Record serial-console settings and the source revision being tested.
5. Hash and archive the exact binaries installed for the experiment.

After a failure, stop the experiment, preserve serial and firmware logs, restore
the known-good image, and confirm firmware setup access, normal boot, operating
system startup, and device functionality before continuing.

## Default experiment boundary

The initial test sequence is limited to health checks, bounded reads, request
validation, timing measurements, and synthetic workloads. Credential access,
third-party software, persistent runtime payload deployment, and unapproved
write operations are outside the default scope.

## Experiment record

Every run should record an identifier, objective, repository revision, binary
hashes, platform configuration, selected source tree, command and buffer
sizes, repetition count, pacing, collected WMI/SMM/serial telemetry, outcome,
and recovery actions.
