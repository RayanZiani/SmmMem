#include <intrin.h>
#include "Common.h"

#pragma intrinsic(__inbyte)
#pragma intrinsic(__outbyte)
#pragma function(memset)
#pragma function(memcpy)

typedef struct {
  UINT32 Signature;
  UINT32 Length;
  UINT8 Revision;
  UINT8 Checksum;
  UINT8 OemId[6];
  UINT8 OemTableId[8];
  UINT32 OemRevision;
  UINT32 CreatorId;
  UINT32 CreatorRevision;
} ACPI_HEADER;

typedef struct {
  UINT32 Type;
  UINT32 Pad;
  UINT64 PhysicalStart;
  UINT64 VirtualStart;
  UINT64 NumberOfPages;
  UINT64 Attribute;
} EFI_MEMORY_DESCRIPTOR;

typedef struct {
  UINT32 Version;
  UINT32 NumberOfEntries;
  UINT32 DescriptorSize;
  UINT32 Reserved;
} EDKII_PI_SMM_COMMUNICATION_REGION_TABLE;

#define EFI_RESERVED_MEMORY_TYPE 0U
#define EFI_CONVENTIONAL_MEMORY 7U
#define EFI_ACPI_MEMORY_NVS 10U

static EFI_SYSTEM_TABLE *gSystemTable;
static EFI_PHYSICAL_ADDRESS gMailboxPhysical;
static EFI_EVENT gRetryTimerEvent;
static EFI_EVENT gReadyToBootEvent;
static EFI_EVENT gSmmCommEvent;
static EFI_EVENT gAcpiEvent;
static VOID *gSmmCommRegistration;
static VOID *gAcpiRegistration;
static UINT32 gConfigured;
static UINT32 gConfigureAttempts;
static UINT32 gWmiInstalled;
static UINT32 gWmiAttempts;
static CONFIG gPublishedConfig;
static DEBUG_STATE gDebugState;
static DEBUG_TRACE gDebugTrace;
static UINT8 gSsdt[512];
static CHAR16 gDebugName[] = {
    'S', 'm', 'm', 'M', 'e', 'm', 'D', 'e', 'b', 'u', 'g', 0};
static CHAR16 gTraceName[] = {
    'S', 'm', 'm', 'M', 'e', 'm', 'T', 'r', 'a', 'c', 'e', 0};

static EFI_GUID gEfiEventReadyToBootGuid = {
    0x7ce88fb3,
    0x4bd7,
    0x4679,
    {0x87, 0xa8, 0xa8, 0xd8, 0xde, 0xe5, 0x0d, 0x2b}};
static EFI_GUID gEdkiiPiSmmCommunicationRegionTableGuid = {
    0x4e28ca50,
    0xd582,
    0x44ac,
    {0xa1, 0x1f, 0xe3, 0xd5, 0x65, 0x26, 0xdb, 0x34}};

static VOID CopyMemLocal(VOID *Destination, const VOID *Source, UINTN Size) {
  UINT8 *Dst = (UINT8 *)Destination;
  const UINT8 *Src = (const UINT8 *)Source;
  while (Size--) {
    *Dst++ = *Src++;
  }
}

static VOID ZeroMem(VOID *Buffer, UINTN Size) {
  UINT8 *Ptr = (UINT8 *)Buffer;
  while (Size--) {
    *Ptr++ = 0;
  }
}

static VOID CopyAscii(char *Destination, const char *Source, UINTN Size) {
  if (Size == 0) {
    return;
  }
  while (Size > 1 && *Source != 0) {
    *Destination++ = *Source++;
    Size--;
  }
  *Destination = 0;
}

static VOID SaveDebug(VOID) {
  UINT32 VolatileAttributes;
  UINT32 TraceAttributes;

  if (gSystemTable == 0 || gSystemTable->RuntimeServices == 0 ||
      gSystemTable->RuntimeServices->SetVariable == 0) {
    return;
  }
  gDebugState.Magic = DEBUG_MAGIC;
  gDebugState.MailboxPhysical = gMailboxPhysical;
  gDebugState.MailboxSize = MAILBOX_SIZE;
  gDebugState.SwSmiValue = SW_SMI_VALUE;
  gDebugState.WmiInstalled = gWmiInstalled;
  gDebugState.SmmConfigured = gConfigured;
  gDebugState.ConfigureAttempts = gConfigureAttempts;
  gDebugTrace.Magic = DEBUG_TRACE_MAGIC;
  gDebugTrace.LastStage = gDebugState.DxeStage;
  gDebugTrace.LastStatus = gDebugState.DxeStatus;
  gDebugTrace.State = gDebugState;
  VolatileAttributes =
      EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS;
  TraceAttributes = EFI_VARIABLE_NON_VOLATILE |
                    EFI_VARIABLE_BOOTSERVICE_ACCESS |
                    EFI_VARIABLE_RUNTIME_ACCESS;
  gSystemTable->RuntimeServices->SetVariable(
      gDebugName, &gDebugGuid, VolatileAttributes, sizeof(gDebugState),
      &gDebugState);
  gSystemTable->RuntimeServices->SetVariable(
      gTraceName, &gDebugGuid, TraceAttributes, sizeof(gDebugTrace),
      &gDebugTrace);
}

static VOID DebugRecord(UINT32 Stage, EFI_STATUS Status, UINT64 Data0,
                        UINT64 Data1, UINT64 Data2) {
  UINT32 Index;

  gDebugState.DxeStage = Stage;
  gDebugState.DxeStatus = (UINT32)Status;
  if (gDebugTrace.RecordCount < DEBUG_RECORD_COUNT) {
    Index = gDebugTrace.RecordCount++;
  } else {
    Index = DEBUG_RECORD_COUNT - 1;
    CopyMemLocal(&gDebugTrace.Records[0], &gDebugTrace.Records[1],
                 sizeof(DEBUG_RECORD) * (DEBUG_RECORD_COUNT - 1));
  }
  gDebugTrace.Records[Index].Stage = Stage;
  gDebugTrace.Records[Index].Status = Status;
  gDebugTrace.Records[Index].Data0 = Data0;
  gDebugTrace.Records[Index].Data1 = Data1;
  gDebugTrace.Records[Index].Data2 = Data2;
  SaveDebug();
}

static VOID DebugInit(VOID) {
  ZeroMem(&gDebugState, sizeof(gDebugState));
  ZeroMem(&gDebugTrace, sizeof(gDebugTrace));
  gDebugTrace.Magic = DEBUG_TRACE_MAGIC;
  CopyAscii(gDebugTrace.Build, __DATE__ " " __TIME__,
            sizeof(gDebugTrace.Build));
}

VOID *memset(VOID *Destination, int Value, size_t Size) {
  UINT8 *Dst = (UINT8 *)Destination;
  while (Size--) {
    *Dst++ = (UINT8)Value;
  }
  return Destination;
}

VOID *memcpy(VOID *Destination, const VOID *Source, size_t Size) {
  CopyMemLocal(Destination, Source, Size);
  return Destination;
}

static BOOLEAN CompareGuid(const EFI_GUID *A, const EFI_GUID *B) {
  const UINT8 *Ap = (const UINT8 *)A;
  const UINT8 *Bp = (const UINT8 *)B;
  UINTN Index;
  for (Index = 0; Index < sizeof(EFI_GUID); Index++) {
    if (Ap[Index] != Bp[Index]) {
      return 0;
    }
  }
  return 1;
}

#if SERIAL
static VOID IoWait(VOID) { __outbyte(0x80, 0); }

static VOID SerialInit(VOID) {
  __outbyte(COM1_PORT + 1, 0x00);
  __outbyte(COM1_PORT + 3, 0x80);
  __outbyte(COM1_PORT + 0, 0x01);
  __outbyte(COM1_PORT + 1, 0x00);
  __outbyte(COM1_PORT + 3, 0x03);
  __outbyte(COM1_PORT + 2, 0xC7);
  __outbyte(COM1_PORT + 4, 0x0B);
  IoWait();
}

static VOID SerialPutChar(char Ch) {
  UINTN Guard = 100000;
  if (Ch == '\n') {
    SerialPutChar('\r');
  }
  while (((__inbyte(COM1_PORT + 5) & 0x20) == 0) && Guard--) {
  }
  __outbyte(COM1_PORT, (UINT8)Ch);
}

static VOID Log(const char *Text) {
  while (*Text) {
    SerialPutChar(*Text++);
  }
}

static VOID LogHex(UINT64 Value) {
  char Hex[] = "0123456789ABCDEF";
  UINTN Shift = 60;
  for (;;) {
    SerialPutChar(Hex[(Value >> Shift) & 0xFU]);
    if (Shift == 0) {
      break;
    }
    Shift -= 4;
  }
}

static VOID LogStatus(const char *Text, EFI_STATUS Status) {
  Log(Text);
  LogHex(Status);
  Log("\n");
}
#else
#define SerialInit()
#define Log(Text)
#define LogHex(Value)
#define LogStatus(Text, Status)
#endif

static UINT32 Signature32(char A, char B, char C, char D) {
  return (UINT32)(UINT8)A | ((UINT32)(UINT8)B << 8) |
         ((UINT32)(UINT8)C << 16) | ((UINT32)(UINT8)D << 24);
}

static UINT8 AcpiChecksum(const UINT8 *Buffer, UINTN Size) {
  UINT8 Sum = 0;
  while (Size--) {
    Sum = (UINT8)(Sum + *Buffer++);
  }
  return (UINT8)(0 - Sum);
}

static UINT8 *EmitName(UINT8 *Out, const char Name[4]) {
  CopyMemLocal(Out, Name, 4);
  return Out + 4;
}

static UINT8 *EmitBytes(UINT8 *Out, const VOID *Data, UINTN Size) {
  CopyMemLocal(Out, Data, Size);
  return Out + Size;
}

static UINT8 *EmitDwordConst(UINT8 *Out, UINT32 Value) {
  *Out++ = 0x0C;
  *Out++ = (UINT8)Value;
  *Out++ = (UINT8)(Value >> 8);
  *Out++ = (UINT8)(Value >> 16);
  *Out++ = (UINT8)(Value >> 24);
  return Out;
}

static UINT8 *EmitString(UINT8 *Out, const char *Text) {
  *Out++ = 0x0D;
  while (*Text) {
    *Out++ = (UINT8)*Text++;
  }
  *Out++ = 0;
  return Out;
}

static UINT8 *ReservePkgLen(UINT8 *Out) {
  *Out++ = 0;
  *Out++ = 0;
  return Out;
}

static UINT8 *EmitPkgLength(UINT8 *Out, UINTN Length) {
  if (Length < 0x40U) {
    *Out++ = (UINT8)Length;
  } else if (Length < 0x1000U) {
    *Out++ = (UINT8)(0x40U | (Length & 0x0FU));
    *Out++ = (UINT8)((Length >> 4) & 0xFFU);
  } else if (Length < 0x100000U) {
    *Out++ = (UINT8)(0x80U | (Length & 0x0FU));
    *Out++ = (UINT8)((Length >> 4) & 0xFFU);
    *Out++ = (UINT8)((Length >> 12) & 0xFFU);
  } else {
    *Out++ = (UINT8)(0xC0U | (Length & 0x0FU));
    *Out++ = (UINT8)((Length >> 4) & 0xFFU);
    *Out++ = (UINT8)((Length >> 12) & 0xFFU);
    *Out++ = (UINT8)((Length >> 20) & 0xFFU);
  }
  return Out;
}

static VOID PatchPkgLen(UINT8 *Pkg, UINTN Length) {
  Pkg[0] = (UINT8)(0x40U | (Length & 0x0FU));
  Pkg[1] = (UINT8)((Length >> 4) & 0xFFU);
}

static UINTN BuildSsdt(UINT8 *Buffer, UINTN Capacity) {
  ACPI_HEADER *Header = (ACPI_HEADER *)Buffer;
  UINT8 *Out = Buffer + sizeof(ACPI_HEADER);
  UINT8 *ScopePkg;
  UINT8 *DevicePkg;
  UINT8 *ReqFieldPkg;
  UINT8 *OutFieldPkg;
  UINT8 *MethodPkg;
  UINT8 Wdg[20];

  if (Capacity < sizeof(gSsdt)) {
    return 0;
  }
  ZeroMem(Buffer, Capacity);
  Header->Signature = Signature32('S', 'S', 'D', 'T');
  Header->Revision = 2;
  CopyMemLocal(Header->OemId, "MEMDEV", 6);
  CopyMemLocal(Header->OemTableId, "MEMDEV  ", 8);
  Header->OemRevision = 1;
  Header->CreatorId = Signature32('S', 'M', 'M', 'M');
  Header->CreatorRevision = 1;

  *Out++ = 0x10;
  ScopePkg = Out;
  Out = ReservePkgLen(Out);
  *Out++ = 0x5C;
  Out = EmitName(Out, "_SB_");

  *Out++ = 0x5B;
  *Out++ = 0x82;
  DevicePkg = Out;
  Out = ReservePkgLen(Out);
  Out = EmitName(Out, "SMMM");

  *Out++ = 0x08;
  Out = EmitName(Out, "_HID");
  Out = EmitString(Out, "PNP0C14");
  *Out++ = 0x08;
  Out = EmitName(Out, "_UID");
  Out = EmitString(Out, "Mem");

  ZeroMem(Wdg, sizeof(Wdg));
  CopyMemLocal(Wdg, &gWmiGuid, sizeof(gWmiGuid));
  Wdg[16] = 'B';
  Wdg[17] = 'D';
  Wdg[18] = 1;
  Wdg[19] = 0x02;
  *Out++ = 0x08;
  Out = EmitName(Out, "_WDG");
  *Out++ = 0x11;
  *Out++ = 0x17;
  *Out++ = 0x0A;
  *Out++ = (UINT8)sizeof(Wdg);
  Out = EmitBytes(Out, Wdg, sizeof(Wdg));

  *Out++ = 0x5B;
  *Out++ = 0x80;
  Out = EmitName(Out, "SMIR");
  *Out++ = 0x01;
  Out = EmitDwordConst(Out, 0xB2U);
  *Out++ = 0x0A;
  *Out++ = 0x01;

  *Out++ = 0x5B;
  *Out++ = 0x81;
  *Out++ = 0x0B;
  Out = EmitName(Out, "SMIR");
  *Out++ = 0x01;
  Out = EmitName(Out, "SMIC");
  *Out++ = 0x08;

  *Out++ = 0x5B;
  *Out++ = 0x80;
  Out = EmitName(Out, "MREQ");
  *Out++ = 0x00;
  Out = EmitDwordConst(Out, (UINT32)gMailboxPhysical);
  Out = EmitDwordConst(Out, REQUEST_SIZE);

  *Out++ = 0x5B;
  *Out++ = 0x81;
  ReqFieldPkg = Out;
  Out = ReservePkgLen(Out);
  Out = EmitName(Out, "MREQ");
  *Out++ = 0x01;
  Out = EmitName(Out, "WMRQ");
  Out = EmitPkgLength(Out, REQUEST_SIZE * 8U);
  PatchPkgLen(ReqFieldPkg, (UINTN)(Out - ReqFieldPkg));

  *Out++ = 0x5B;
  *Out++ = 0x80;
  Out = EmitName(Out, "MOUT");
  *Out++ = 0x00;
  Out = EmitDwordConst(Out,
                       (UINT32)(gMailboxPhysical + RESPONSE_OFFSET));
  Out = EmitDwordConst(Out, RESPONSE_SIZE);

  *Out++ = 0x5B;
  *Out++ = 0x81;
  OutFieldPkg = Out;
  Out = ReservePkgLen(Out);
  Out = EmitName(Out, "MOUT");
  *Out++ = 0x01;
  Out = EmitName(Out, "WOUT");
  Out = EmitPkgLength(Out, RESPONSE_SIZE * 8U);
  PatchPkgLen(OutFieldPkg, (UINTN)(Out - OutFieldPkg));

  *Out++ = 0x14;
  MethodPkg = Out;
  Out = ReservePkgLen(Out);
  Out = EmitName(Out, "WMBD");
  *Out++ = 0x03;
  *Out++ = 0x70;
  *Out++ = 0x6A;
  Out = EmitName(Out, "WMRQ");
  *Out++ = 0x70;
  *Out++ = 0x0A;
  *Out++ = (UINT8)SW_SMI_VALUE;
  Out = EmitName(Out, "SMIC");
  *Out++ = 0xA4;
  Out = EmitName(Out, "WOUT");

  PatchPkgLen(MethodPkg, (UINTN)(Out - MethodPkg));
  PatchPkgLen(DevicePkg, (UINTN)(Out - DevicePkg));
  PatchPkgLen(ScopePkg, (UINTN)(Out - ScopePkg));
  Header->Length = (UINT32)(Out - Buffer);
  Header->Checksum = AcpiChecksum(Buffer, Header->Length);
  return Header->Length;
}

static EFI_STATUS AllocateMailbox(VOID) {
  EFI_STATUS Status;
  UINTN Pages = (MAILBOX_SIZE + 0xFFFU) / 0x1000U;

  if (gMailboxPhysical != 0) {
    return EFI_SUCCESS;
  }
  Status = gSystemTable->BootServices->AllocatePages(
      AllocateAnyPages, EFI_RUNTIME_SERVICES_DATA, Pages, &gMailboxPhysical);
  if (EFI_ERROR(Status)) {
    return Status;
  }
  ZeroMem((VOID *)(UINTN)gMailboxPhysical, MAILBOX_SIZE);
  return EFI_SUCCESS;
}

static EFI_STATUS InstallWmi(VOID) {
  EFI_ACPI_TABLE_PROTOCOL *AcpiTable = 0;
  EFI_STATUS Status;
  UINTN TableKey;
  UINTN TableSize;
  UINT32 Attempt;
  BOOLEAN LogIt;

  if (gWmiInstalled != 0) {
    return EFI_SUCCESS;
  }
  Attempt = ++gWmiAttempts;
  LogIt = (Attempt <= 3);
  if (LogIt) {
    Log("dxe install wmi attempt=0x");
    LogHex(Attempt);
    Log("\n");
  }
  Status = gSystemTable->BootServices->LocateProtocol(
      &gEfiAcpiTableProtocolGuid, 0, (VOID **)&AcpiTable);
  if (EFI_ERROR(Status) || AcpiTable == 0) {
    DebugRecord(DBG_DXE_ACPI_LOCATE,
                EFI_ERROR(Status) ? Status : EFI_NOT_FOUND,
                (UINT64)(UINTN)AcpiTable, Attempt, 0);
    if (LogIt) {
      LogStatus("dxe acpi protocol unavailable ",
                EFI_ERROR(Status) ? Status : EFI_NOT_FOUND);
    }
    return EFI_ERROR(Status) ? Status : EFI_NOT_FOUND;
  }
  DebugRecord(DBG_DXE_ACPI_LOCATE, EFI_SUCCESS, (UINT64)(UINTN)AcpiTable,
              Attempt, 0);
  TableSize = BuildSsdt(gSsdt, sizeof(gSsdt));
  if (TableSize == 0) {
    DebugRecord(DBG_DXE_SSDT_BUILT, EFI_OUT_OF_RESOURCES,
                sizeof(gSsdt), Attempt, 0);
    if (LogIt) {
      Log("dxe ssdt build failed\n");
    }
    return EFI_OUT_OF_RESOURCES;
  }
  DebugRecord(DBG_DXE_SSDT_BUILT, EFI_SUCCESS, TableSize, Attempt, 0);
  if (LogIt) {
    Log("dxe ssdt size=0x");
    LogHex(TableSize);
    Log("\n");
  }
  Status = AcpiTable->InstallAcpiTable(AcpiTable, gSsdt, TableSize,
                                       &TableKey);
  if (EFI_ERROR(Status)) {
    DebugRecord(DBG_DXE_SSDT_INSTALLED, Status, TableSize, Attempt, 0);
    if (LogIt) {
      LogStatus("dxe ssdt install failed ", Status);
    }
  } else {
    gWmiInstalled = 1;
    DebugRecord(DBG_DXE_SSDT_INSTALLED, EFI_SUCCESS, TableSize, TableKey,
                Attempt);
    Log("dxe ssdt installed\n");
  }
  return Status;
}

static EFI_STATUS PublishConfig(VOID) {
  EFI_STATUS Status;

  if (gSystemTable->BootServices->InstallConfigurationTable == 0) {
    return EFI_UNSUPPORTED;
  }
  ZeroMem(&gPublishedConfig, sizeof(gPublishedConfig));
  gPublishedConfig.Magic = CONFIG_MAGIC;
  gPublishedConfig.MailboxPhysical = gMailboxPhysical;
  gPublishedConfig.MailboxSize = MAILBOX_SIZE;
  gPublishedConfig.SwSmiValue = SW_SMI_VALUE;
  Status = gSystemTable->BootServices->InstallConfigurationTable(
      &gConfigGuid, &gPublishedConfig);
  DebugRecord(DBG_DXE_CONFIG_PUBLISHED, Status, gMailboxPhysical,
              MAILBOX_SIZE, SW_SMI_VALUE);
  return Status;
}

static VOID *FindSmmCommRegion(UINTN RequiredSize, UINT32 *OriginalType,
                               BOOLEAN LogIt) {
  EDKII_PI_SMM_COMMUNICATION_REGION_TABLE *Table = 0;
  UINT8 *Entry;
  UINTN Index;

  *OriginalType = 0xFFFFFFFFU;
  if (gSystemTable == 0 || gSystemTable->ConfigurationTable == 0) {
    if (LogIt) {
      Log("dxe config table unavailable\n");
    }
    return 0;
  }
  for (Index = 0; Index < gSystemTable->NumberOfTableEntries; Index++) {
    EFI_CONFIGURATION_TABLE *Config = &gSystemTable->ConfigurationTable[Index];
    if (CompareGuid(&Config->VendorGuid,
                    &gEdkiiPiSmmCommunicationRegionTableGuid)) {
      Table = (EDKII_PI_SMM_COMMUNICATION_REGION_TABLE *)Config->VendorTable;
      break;
    }
  }
  if (Table == 0 || Table->Version != 1 || Table->NumberOfEntries == 0 ||
      Table->DescriptorSize < sizeof(EFI_MEMORY_DESCRIPTOR)) {
    if (LogIt) {
      if (Table == 0) {
        Log("dxe smm comm region table missing\n");
      } else {
        Log("dxe smm comm region table rejected version=0x");
        LogHex(Table->Version);
        Log(" entries=0x");
        LogHex(Table->NumberOfEntries);
        Log(" desc=0x");
        LogHex(Table->DescriptorSize);
        Log("\n");
      }
    }
    return 0;
  }
  Entry = (UINT8 *)(Table + 1);
  for (Index = 0; Index < Table->NumberOfEntries; Index++) {
    EFI_MEMORY_DESCRIPTOR *Desc =
        (EFI_MEMORY_DESCRIPTOR *)(Entry + (Index * Table->DescriptorSize));
    UINT64 Bytes = Desc->NumberOfPages << 12;
    if (Bytes < RequiredSize) {
      continue;
    }
    if (Desc->Type == EFI_RESERVED_MEMORY_TYPE ||
        Desc->Type == EFI_RUNTIME_SERVICES_DATA ||
        Desc->Type == EFI_ACPI_MEMORY_NVS ||
        Desc->Type == EFI_CONVENTIONAL_MEMORY) {
      *OriginalType = Desc->Type;
      if (LogIt) {
        Log("dxe smm comm region base=0x");
        LogHex(Desc->PhysicalStart);
        Log(" size=0x");
        LogHex(Bytes);
        Log(" type=0x");
        LogHex(*OriginalType);
        Log("\n");
      }
      if (Desc->Type == EFI_CONVENTIONAL_MEMORY) {
        Desc->Type = EFI_RESERVED_MEMORY_TYPE;
      }
      return (VOID *)(UINTN)Desc->PhysicalStart;
    }
  }
  if (LogIt) {
    Log("dxe no usable smm comm region\n");
  }
  return 0;
}

static VOID RestoreSmmCommRegionType(VOID *CommBuffer, UINT32 OriginalType) {
  EDKII_PI_SMM_COMMUNICATION_REGION_TABLE *Table = 0;
  UINT8 *Entry;
  UINTN Index;

  if (CommBuffer == 0 || OriginalType != EFI_CONVENTIONAL_MEMORY ||
      gSystemTable == 0 || gSystemTable->ConfigurationTable == 0) {
    return;
  }
  for (Index = 0; Index < gSystemTable->NumberOfTableEntries; Index++) {
    EFI_CONFIGURATION_TABLE *Config = &gSystemTable->ConfigurationTable[Index];
    if (CompareGuid(&Config->VendorGuid,
                    &gEdkiiPiSmmCommunicationRegionTableGuid)) {
      Table = (EDKII_PI_SMM_COMMUNICATION_REGION_TABLE *)Config->VendorTable;
      break;
    }
  }
  if (Table == 0) {
    return;
  }
  Entry = (UINT8 *)(Table + 1);
  for (Index = 0; Index < Table->NumberOfEntries; Index++) {
    EFI_MEMORY_DESCRIPTOR *Desc =
        (EFI_MEMORY_DESCRIPTOR *)(Entry + (Index * Table->DescriptorSize));
    if ((VOID *)(UINTN)Desc->PhysicalStart == CommBuffer) {
      Desc->Type = OriginalType;
      return;
    }
  }
}

static EFI_STATUS ConfigureSmm(VOID) {
  EFI_SMM_COMMUNICATION_PROTOCOL *SmmComm = 0;
  EFI_SMM_COMMUNICATE_HEADER *Header;
  CONFIG *Config;
  VOID *CommBuffer;
  UINT32 OriginalRegionType;
  UINT32 Attempt;
  BOOLEAN LogIt;
  EFI_STATUS Status;
  UINTN CommSize;

  Attempt = ++gConfigureAttempts;
  LogIt = (Attempt <= 3);
  if (LogIt) {
    Log("dxe configure smm attempt=0x");
    LogHex(Attempt);
    Log("\n");
  }
  Status = gSystemTable->BootServices->LocateProtocol(
      &gEfiSmmCommunicationProtocolGuid, 0, (VOID **)&SmmComm);
  if (EFI_ERROR(Status) || SmmComm == 0) {
    DebugRecord(DBG_DXE_SMM_LOCATE,
                EFI_ERROR(Status) ? Status : EFI_NOT_FOUND,
                (UINT64)(UINTN)SmmComm, Attempt, 0);
    if (LogIt) {
      LogStatus("dxe smm communication unavailable ",
                EFI_ERROR(Status) ? Status : EFI_NOT_FOUND);
    }
    return EFI_ERROR(Status) ? Status : EFI_NOT_FOUND;
  }
  DebugRecord(DBG_DXE_SMM_LOCATE, EFI_SUCCESS, (UINT64)(UINTN)SmmComm,
              Attempt, 0);
  CommSize = sizeof(EFI_SMM_COMMUNICATE_HEADER) + sizeof(CONFIG) + 16;
  CommBuffer = FindSmmCommRegion(CommSize, &OriginalRegionType, LogIt);
  if (CommBuffer == 0) {
    DebugRecord(DBG_DXE_SMM_COMM_REGION, EFI_NOT_FOUND, CommSize, Attempt, 0);
    if (LogIt) {
      Log("dxe smm configure no comm buffer\n");
    }
    return EFI_NOT_FOUND;
  }
  DebugRecord(DBG_DXE_SMM_COMM_REGION, EFI_SUCCESS,
              (UINT64)(UINTN)CommBuffer, CommSize, OriginalRegionType);
  ZeroMem(CommBuffer, CommSize);
  Header = (EFI_SMM_COMMUNICATE_HEADER *)CommBuffer;
  Header->HeaderGuid = gConfigCommGuid;
  Header->MessageLength = sizeof(CONFIG);
  Config = (CONFIG *)Header->Data;
  Config->Magic = CONFIG_MAGIC;
  Config->MailboxPhysical = gMailboxPhysical;
  Config->MailboxSize = MAILBOX_SIZE;
  Config->SwSmiValue = SW_SMI_VALUE;
  Status = SmmComm->Communicate(SmmComm, Header, &CommSize);
  RestoreSmmCommRegionType(CommBuffer, OriginalRegionType);
  if (!EFI_ERROR(Status)) {
    gConfigured = 1;
    DebugRecord(DBG_DXE_SMM_CONFIGURED, EFI_SUCCESS, CommSize, Attempt, 0);
    Log("dxe smm configure ok\n");
  } else {
    DebugRecord(DBG_DXE_SMM_CONFIGURED, Status, CommSize, Attempt, 0);
    if (LogIt) {
      LogStatus("dxe smm communicate failed ", Status);
      Log("dxe smm communicate size=0x");
      LogHex(CommSize);
      Log("\n");
    }
  }
  return Status;
}

static VOID EFIAPI RetrySetup(EFI_EVENT Event, VOID *Context) {
  EFI_STATUS WmiStatus;
  EFI_STATUS SmmStatus;
  (void)Event;
  (void)Context;

  if (gWmiInstalled != 0 && gConfigured != 0) {
    return;
  }
  WmiStatus = EFI_SUCCESS;
  SmmStatus = EFI_SUCCESS;
  if (gWmiInstalled == 0) {
    WmiStatus = InstallWmi();
  }
  if (gConfigured == 0) {
    SmmStatus = ConfigureSmm();
  }
  if (!EFI_ERROR(WmiStatus) && !EFI_ERROR(SmmStatus) &&
      gRetryTimerEvent != 0) {
    SmmStatus = gSystemTable->BootServices->SetTimer(gRetryTimerEvent,
                                                     TimerCancel, 0);
    if (EFI_ERROR(SmmStatus)) {
      LogStatus("dxe retry timer cancel failed ", SmmStatus);
    } else {
      Log("dxe retry timer canceled\n");
    }
  }
}

static VOID RegisterRetryTimer(VOID) {
  EFI_STATUS Status;

  if (gRetryTimerEvent != 0 || (gWmiInstalled != 0 && gConfigured != 0)) {
    return;
  }
  Status = gSystemTable->BootServices->CreateEvent(
      EVT_TIMER | EVT_NOTIFY_SIGNAL, TPL_CALLBACK, RetrySetup, 0,
      &gRetryTimerEvent);
  if (EFI_ERROR(Status)) {
    gRetryTimerEvent = 0;
    LogStatus("dxe retry timer create failed ", Status);
    return;
  }
  Status = gSystemTable->BootServices->SetTimer(gRetryTimerEvent,
                                                TimerPeriodic, 5000000ULL);
  if (EFI_ERROR(Status)) {
    gSystemTable->BootServices->CloseEvent(gRetryTimerEvent);
    gRetryTimerEvent = 0;
    LogStatus("dxe retry timer start failed ", Status);
  } else {
    Log("dxe retry timer registered\n");
  }
}

static VOID RegisterSmmCommNotify(VOID) {
  EFI_STATUS Status;

  if (gSmmCommEvent != 0 || gConfigured != 0) {
    return;
  }
  Status = gSystemTable->BootServices->CreateEvent(
      EVT_NOTIFY_SIGNAL, TPL_CALLBACK, RetrySetup, 0, &gSmmCommEvent);
  if (EFI_ERROR(Status)) {
    gSmmCommEvent = 0;
    LogStatus("dxe smm communication notify create failed ", Status);
    return;
  }
  Status = gSystemTable->BootServices->RegisterProtocolNotify(
      &gEfiSmmCommunicationProtocolGuid, gSmmCommEvent,
      &gSmmCommRegistration);
  if (EFI_ERROR(Status)) {
    gSystemTable->BootServices->CloseEvent(gSmmCommEvent);
    gSmmCommEvent = 0;
    LogStatus("dxe smm communication notify failed ", Status);
  } else {
    Log("dxe smm communication notify registered\n");
  }
}

static VOID RegisterAcpiNotify(VOID) {
  EFI_STATUS Status;

  if (gAcpiEvent != 0 || gWmiInstalled != 0) {
    return;
  }
  Status = gSystemTable->BootServices->CreateEvent(
      EVT_NOTIFY_SIGNAL, TPL_CALLBACK, RetrySetup, 0, &gAcpiEvent);
  if (EFI_ERROR(Status)) {
    gAcpiEvent = 0;
    LogStatus("dxe acpi notify create failed ", Status);
    return;
  }
  Status = gSystemTable->BootServices->RegisterProtocolNotify(
      &gEfiAcpiTableProtocolGuid, gAcpiEvent, &gAcpiRegistration);
  if (EFI_ERROR(Status)) {
    gSystemTable->BootServices->CloseEvent(gAcpiEvent);
    gAcpiEvent = 0;
    LogStatus("dxe acpi notify failed ", Status);
  } else {
    Log("dxe acpi notify registered\n");
  }
}

static VOID RegisterReadyToBootRetry(VOID) {
  EFI_STATUS Status;

  if (gReadyToBootEvent != 0 || (gWmiInstalled != 0 && gConfigured != 0) ||
      gSystemTable->BootServices->CreateEventEx == 0) {
    if (gSystemTable->BootServices->CreateEventEx == 0) {
      Log("dxe ready retry unavailable\n");
    }
    return;
  }
  Status = gSystemTable->BootServices->CreateEventEx(
      EVT_NOTIFY_SIGNAL, TPL_CALLBACK, RetrySetup, 0,
      &gEfiEventReadyToBootGuid, &gReadyToBootEvent);
  if (EFI_ERROR(Status)) {
    gReadyToBootEvent = 0;
    LogStatus("dxe ready retry failed ", Status);
  } else {
    Log("dxe ready retry registered\n");
  }
}

EFI_STATUS EFIAPI DxeEntry(EFI_HANDLE ImageHandle,
                           EFI_SYSTEM_TABLE *SystemTable) {
  EFI_STATUS Status;
  (void)ImageHandle;

  gSystemTable = SystemTable;
  DebugInit();
  SerialInit();
  DebugRecord(DBG_DXE_ENTRY, EFI_SUCCESS, 0, 0, 0);
  Log("mem dxe init\n");
  Status = AllocateMailbox();
  if (EFI_ERROR(Status)) {
    DebugRecord(DBG_DXE_MAILBOX_OK, Status, MAILBOX_SIZE, 0, 0);
    LogStatus("dxe mailbox failed ", Status);
    return EFI_SUCCESS;
  }
  DebugRecord(DBG_DXE_MAILBOX_OK, EFI_SUCCESS, gMailboxPhysical,
              MAILBOX_SIZE, SW_SMI_VALUE);
  Log("dxe mailbox base=0x");
  LogHex(gMailboxPhysical);
  Log(" size=0x");
  LogHex(MAILBOX_SIZE);
  Log(" sw=0x");
  LogHex(SW_SMI_VALUE);
  Log("\n");
  Status = PublishConfig();
  if (EFI_ERROR(Status)) {
    LogStatus("dxe config publish failed ", Status);
  } else {
    Log("dxe config published\n");
  }
  Status = InstallWmi();
  if (EFI_ERROR(Status)) {
    LogStatus("dxe wmi install failed ", Status);
  }
  Status = ConfigureSmm();
  if (gWmiInstalled == 0 || gConfigured == 0) {
    RegisterRetryTimer();
    RegisterReadyToBootRetry();
    RegisterSmmCommNotify();
    RegisterAcpiNotify();
  }
  DebugRecord(DBG_DXE_DONE, EFI_SUCCESS, gWmiInstalled, gConfigured,
              gConfigureAttempts);
  Log("mem dxe done\n");
  return EFI_SUCCESS;
}
