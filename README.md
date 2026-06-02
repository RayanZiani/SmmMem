# SmmMem

Driverless Windows memory access through System Management Mode (SMM) with a user-mode API.

## Overview

SmmMem exposes a small Windows client API that can read, write, translate, and resolve memory without installing a kernel driver. The user-mode client talks to an ACPI WMI method, which copies a request into a shared mailbox and rings a software SMI doorbell. The SMM handler then performs the requested operation from ring -2 and writes the response back to the mailbox.

The repository contains two firmware builds:

- `src`: release-oriented firmware and client
- `src_dbg01`: debug build with extra firmware tracing and a Windows debug reader

## Repository Layout

### `src`

- `Dxe.c`: allocates the mailbox, publishes configuration, builds and installs the ACPI SSDT/WMI device, and attempts to configure the SMM side through `EFI_SMM_COMMUNICATION_PROTOCOL`
- `Smm.c`: registers the configuration communication handler and software SMI handler, translates addresses, walks page tables, locates processes/modules/exports, and services requests from the mailbox
- `Client.c`: user-mode transport and API implementation based on `Advapi32!WmiOpenBlock` and `WmiExecuteMethodW`
- `Api.h`: public C API for user applications
- `Common.h`: shared protocol, structure, GUID, command, and firmware type definitions
- `build.cmd`: builds `Dxe.efi`, `Smm.efi`, and the sample client

### `src_dbg01`

The debug tree mirrors the release tree and adds firmware instrumentation:

- `DbgRead.c`: Windows utility that reads firmware debug variables, scans ACPI tables for the SmmMem WMI markers, and sends a WMI ping
- `Dxe.c`: debug-capable DXE that stores installation/configuration progress in firmware variables
- `Smm.c`: debug-capable SMM module that stores SMM initialization and configuration state in firmware variables
- `Common.h`: extended with debug structures, stage identifiers, and the debug variable GUID
- `build.cmd`: builds `DbgRead.exe` in addition to the normal binaries

## Architecture

SmmMem is split into two firmware components and one Windows client:

1. **DXE module (`Dxe.efi`)**
   - Allocates a 0x2000-byte runtime mailbox
   - Publishes mailbox configuration through a UEFI configuration table
   - Builds an SSDT that exposes an ACPI WMI device (`PNP0C14`)
   - Installs the ACPI table and retries setup if ACPI or SMM communication is not ready yet
   - Sends the mailbox configuration to SMM through `EFI_SMM_COMMUNICATION_PROTOCOL`

2. **SMM module (`Smm.efi`)**
   - Locates SMST and registers a config communication handler
   - Accepts mailbox configuration from either the communication buffer or the published configuration table
   - Registers a software SMI handler for the configured SW SMI value (`0xD6`)
   - Services memory and symbol requests directly from SMM

3. **Windows user-mode client**
   - Opens the WMI block with `WmiOpenBlock`
   - Executes ACPI WMI method ID `1` with `WmiExecuteMethodW`
   - Uses a fixed request/response layout shared with firmware

## Request Flow

The effective flow implemented by the source tree is:

`Usermode application -> WmiExecuteMethodW -> ACPI WMI method -> shared mailbox -> software SMI -> SMM handler -> response mailbox -> usermode`

You can embed the provided diagram in environments that render Markdown images:

![SmmMem communication flow](https://github.com/user-attachments/assets/c4baf141-250f-4b38-975f-79fa4cb5a19e)

## Communication Details

The shared protocol is defined in `Common.h` and mirrored in `Client.c`:

- Mailbox size: `0x2000`
- Request buffer: first `0x1000`
- Response buffer: starts at offset `0x1000`
- SW SMI value: `0xD6`
- WMI GUID: `A0C9F8DE-0B71-42A8-B967-E538EACB6F21`

Supported commands:

- `CMD_PING`
- `CMD_READ_PHYS`
- `CMD_WRITE_PHYS`
- `CMD_TRANSLATE_VIRT`
- `CMD_READ_VIRT`
- `CMD_WRITE_VIRT`
- `CMD_FIND_PROCESS_PID`
- `CMD_FIND_PROCESS_NAME`
- `CMD_FIND_MODULE`
- `CMD_FIND_KERNEL_MODULE`
- `CMD_FIND_EXPORT`

## What the SMM Side Does

The SMM handler in `src/Smm.c` and `src_dbg01/Smm.c` implements:

- physical memory reads and writes through SMM CPU I/O services
- CR3-based virtual-to-physical translation
- process discovery by PID or image name
- process CR3 and image base discovery
- user-module enumeration through the PEB loader lists
- kernel-module enumeration through `PsLoadedModuleList`
- export resolution from a selected module image

The implementation dynamically resolves several Windows structure offsets instead of hardcoding a single OS build layout.

## User-Mode API

The public API is declared in `Api.h`:

```c
Init();
Close();
Ping();

FindProcessByPid(pid, &process);
FindProcessByName("notepad.exe", &process);

TranslateVirt(pid, va, &pa);

ReadVirt(pid, va, buffer, size);
WriteVirt(pid, va, buffer, size);

ReadPhys(pa, buffer, size);
WritePhys(pa, buffer, size);

FindModule(&process, "module.dll", &module);
FindKernelModule("ntoskrnl.exe", &module);
FindExport(&module, "PsInitialSystemProcess", &address);

Dump(&module, callback, context);
```

Basic example:

```c
#include "Api.h"

int main(void) {
    PROCESS_INFO process = {0};
    char buffer[16] = {0};

    Init();
    FindProcessByName("notepad.exe", &process);
    ReadVirt(process.Pid, process.ImageBase, buffer, sizeof(buffer));
    Close();
    return 0;
}
```

When using `Client.c` as a library, define `API_ONLY` so the built-in sample `wmain` is excluded.

## Building

Open an **x64 Visual Studio Developer Command Prompt** and run one of the build scripts:

### Release tree

```bat
src\build.cmd
```

Run this command from the repository root.

Build output:

- `Work\build\Smm.efi`
- `Work\build\Dxe.efi`
- `Work\build\Client.exe`

### Debug tree

```bat
build.cmd
```

Run this command from the `src_dbg01` directory.

Build output:

- `Work\build\Smm.efi`
- `Work\build\Dxe.efi`
- `Work\build\Client.exe`
- `Work\build\DbgRead.exe`

For a custom Windows application, compile your code together with `Client.c` and define `API_ONLY`, for example:

```bat
cl /nologo /W4 /O2 /DUNICODE /D_UNICODE /DAPI_ONLY app.c src\Client.c
```

## Firmware Installation

1. Build the DXE and SMM binaries.
2. Insert `Dxe.efi` and `Smm.efi` into firmware for a target board that supports PI SMM.
3. Flash the modified firmware.
4. Boot Windows and monitor serial output for mailbox allocation, SSDT/WMI installation, SMM configuration, and SW SMI registration.
5. Run a user-mode client that calls `Init()` before issuing requests.

The DXE module is responsible for the mailbox and ACPI/WMI doorbell. The SMM module performs the actual memory work.

## Platform Notes

The codebase is written for:

- x64 UEFI firmware
- PI SMM implementations similar to AMI Aptio V environments
- Windows 10 or Windows 11 on the target OS side

The original project notes mention testing on an ASUS TUF X870 / AMD AM5 platform, with expected portability to Intel platforms that expose equivalent SMM services.

## Debug Build (`src_dbg01`)

The debug firmware stores progress in UEFI variables using GUID `8EF7C961-13F3-4574-B417-7D99A1A52A8D`.

Important variables:

- `SmmMemDebug`: DXE-side state
- `SmmMemSmmDebug`: SMM-side state
- `SmmMemTrace`: rolling trace of DXE stages

`DbgRead.exe`:

- enables `SeSystemEnvironmentPrivilege`
- reads those firmware variables with `GetFirmwareEnvironmentVariableExW`
- prints decoded stage names and status values
- scans ACPI tables for `SMMM`, `MEMDEV`, `WMBD`, and the WMI GUID markers
- issues a `CMD_PING` over WMI to confirm that the doorbell path works

Tracked debug stages include mailbox allocation, ACPI protocol lookup, SSDT generation and installation, SMM communication discovery, configuration delivery, SMST validation, config handler registration, and SW SMI registration.

## Troubleshooting

If the project does not work on a target board:

- try the debug firmware from `src_dbg01`
- capture serial logs from the DXE and SMM modules
- run `DbgRead.exe` from an elevated Windows session
- record the motherboard model, firmware patching method, and whether ACPI markers and WMI ping succeeded

Common failure points visible in the source:

- ACPI table protocol not available when DXE first runs
- no usable SMM communication region table entry
- SMM communication protocol not ready yet
- configuration not accepted by SMM
- SW SMI registration rejected because the chosen SW SMI value is out of range

## Notes

- The project is designed around firmware-based access, not a Windows kernel driver.
- The SMM handler is event-driven; it only runs when an SMI is triggered.
- `Client.c` chunks read and write operations to match the fixed response payload size.
- The debug tree is the best starting point when adapting the project to a new motherboard or firmware layout.
