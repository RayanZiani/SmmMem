#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define DEBUG_RECORD_COUNT 128U
#define WMIGUID_EXECUTE 0x0010
#define WMI_METHOD_ID 1U
#define REQUEST_SIZE 4096U
#define RESPONSE_SIZE 512U
#define RESPONSE_DATA_SIZE 352U
#define REQ_MAGIC 0x5145524D4D5355ULL
#define RESP_MAGIC 0x5345524D4D5355ULL
#define CMD_PING 1U

typedef ULONG(WINAPI *WMI_OPEN_BLOCK)(GUID *Guid, DWORD Access, HANDLE *Block);
typedef ULONG(WINAPI *WMI_EXECUTE_METHOD_W)(HANDLE Block,
                                            const wchar_t *Instance,
                                            ULONG MethodId, ULONG InSize,
                                            void *In, ULONG *OutSize,
                                            void *Out);
typedef ULONG(WINAPI *WMI_CLOSE_BLOCK)(HANDLE Block);

#pragma pack(push, 1)
typedef struct {
  uint64_t Magic;
  uint32_t Command;
  uint32_t DataSize;
  uint64_t Sequence;
  uint64_t Arg1;
  uint64_t Arg2;
  uint64_t Arg3;
  uint8_t Data[1];
} REQUEST;

typedef struct {
  uint64_t Magic;
  uint32_t Status;
  uint32_t Command;
  uint32_t DataSize;
  uint64_t Sequence;
  uint64_t Result;
  uint8_t Data[RESPONSE_DATA_SIZE];
} RESPONSE;

typedef struct {
  uint64_t Magic;
  uint32_t DxeStage;
  uint32_t DxeStatus;
  uint32_t SmmStage;
  uint32_t SmmStatus;
  uint64_t MailboxPhysical;
  uint32_t MailboxSize;
  uint32_t SwSmiValue;
  uint32_t WmiInstalled;
  uint32_t SmmConfigured;
  uint32_t ConfigureAttempts;
  uint32_t Reserved;
} DEBUG_STATE;

typedef struct {
  uint32_t Stage;
  uint32_t Reserved;
  uint64_t Status;
  uint64_t Data0;
  uint64_t Data1;
  uint64_t Data2;
} DEBUG_RECORD;

typedef struct {
  uint64_t Magic;
  char Build[32];
  uint32_t RecordCount;
  uint32_t LastStage;
  uint32_t LastStatus;
  DEBUG_STATE State;
  DEBUG_RECORD Records[DEBUG_RECORD_COUNT];
} DEBUG_TRACE;
#pragma pack(pop)

static const wchar_t *Guid =
    L"{8EF7C961-13F3-4574-B417-7D99A1A52A8D}";

static GUID gMemGuid = {
    0xa0c9f8de,
    0x0b71,
    0x42a8,
    {0xb9, 0x67, 0xe5, 0x38, 0xea, 0xcb, 0x6f, 0x21}};

static const wchar_t *Instances[] = {
    L"ACPI\\PNP0C14\\Mem_0",
    L"ACPI\\PNP0C14\\SMMM_0",
    L"ACPI\\PNP0C14\\0_0",
    L"",
    NULL};

static const char *StageName(uint32_t Stage) {
  switch (Stage) {
  case 0x100: return "DXE entry";
  case 0x110: return "DXE mailbox";
  case 0x120: return "DXE config published";
  case 0x130: return "DXE ACPI locate";
  case 0x140: return "DXE SSDT built";
  case 0x150: return "DXE SSDT installed";
  case 0x160: return "DXE SMM communication locate";
  case 0x170: return "DXE SMM comm region";
  case 0x180: return "DXE SMM configured";
  case 0x1FF: return "DXE done";
  case 0x200: return "SMM entry";
  case 0x210: return "SMM SMST";
  case 0x220: return "SMM config comm";
  case 0x230: return "SMM config applied";
  case 0x240: return "SMM SW SMI";
  case 0x2FF: return "SMM done";
  case 0x300: return "request";
  case 0x310: return "find process name";
  case 0x320: return "find process pid";
  case 0x330: return "resolve layout";
  case 0x340: return "init kernel";
  case 0x350: return "saved CR3";
  case 0x360: return "try CR3";
  case 0x370: return "kernel scan";
  case 0x380: return "copy physical";
  case 0x390: return "translate VA";
  case 0x3A0: return "resolve list";
  case 0x3B0: return "resolve CR3";
  case 0x3C0: return "resolve name";
  case 0x3D0: return "fill process";
  case 0x3E0: return "find module";
  case 0x3F0: return "request done";
  case 0x400: return "SMM map start";
  case 0x410: return "SMM map PML4E";
  case 0x420: return "SMM map PDPTE";
  case 0x430: return "SMM map PDE";
  case 0x440: return "SMM map PTE";
  case 0x450: return "SMM map done";
  case 0x460: return "copy begin";
  case 0x470: return "SMM map enable";
  case 0x480: return "SMM map reload";
  case 0x490: return "window init";
  case 0x4A0: return "window map";
  case 0x4B0: return "window copy";
  case 0x4C0: return "window done";
  default: return "unknown";
  }
}

static int HasBytes(const uint8_t *Data, DWORD Size, const void *Needle,
                    DWORD NeedleSize) {
  DWORD Index;

  for (Index = 0; Index + NeedleSize <= Size; Index++) {
    if (memcmp(Data + Index, Needle, NeedleSize) == 0) {
      return 1;
    }
  }

  return 0;
}

static DWORD Sig(const char Text[4]) {
  return ((DWORD)(uint8_t)Text[0] << 24) |
         ((DWORD)(uint8_t)Text[1] << 16) |
         ((DWORD)(uint8_t)Text[2] << 8) |
         (DWORD)(uint8_t)Text[3];
}

static int EnableFirmwareVariablePrivilege(void) {
  HANDLE Token;
  TOKEN_PRIVILEGES Tp;
  LUID Luid;

  if (!OpenProcessToken(GetCurrentProcess(),
                        TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &Token)) {
    return 0;
  }

  if (!LookupPrivilegeValueW(NULL, L"SeSystemEnvironmentPrivilege", &Luid)) {
    CloseHandle(Token);
    return 0;
  }

  memset(&Tp, 0, sizeof(Tp));
  Tp.PrivilegeCount = 1;
  Tp.Privileges[0].Luid = Luid;
  Tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

  AdjustTokenPrivileges(Token, FALSE, &Tp, sizeof(Tp), NULL, NULL);
  CloseHandle(Token);
  return GetLastError() == ERROR_SUCCESS;
}

static DWORD ReadVariable(const wchar_t *Name, void *Buffer, DWORD Size,
                          DWORD *Attributes) {
  DWORD Read;

  memset(Buffer, 0, Size);
  *Attributes = 0;
  Read = GetFirmwareEnvironmentVariableExW(Name, Guid, Buffer, Size,
                                           Attributes);
  if (Read == 0) {
    printf("%ls: read failed, error=%lu\n", Name, GetLastError());
  }

  return Read;
}

static void PrintState(const char *Name, const DEBUG_STATE *State,
                       DWORD Size, DWORD Attributes) {
  printf("\n%s size=%lu attrs=0x%08lX\n", Name, Size, Attributes);
  printf("  magic=0x%llX\n", State->Magic);
  printf("  dxe=0x%X %-28s status=0x%X\n",
         State->DxeStage, StageName(State->DxeStage), State->DxeStatus);
  printf("  smm=0x%X %-28s status=0x%X\n",
         State->SmmStage, StageName(State->SmmStage), State->SmmStatus);
  printf("  mailbox=0x%llX size=0x%X sw=0x%X\n",
         State->MailboxPhysical, State->MailboxSize, State->SwSmiValue);
  printf("  wmi=%u smm_configured=%u attempts=%u\n",
         State->WmiInstalled, State->SmmConfigured,
         State->ConfigureAttempts);
}

static void PrintTrace(const char *Name, const DEBUG_TRACE *Trace, DWORD Size,
                       DWORD Attributes) {
  uint32_t Count;
  uint32_t Index;

  printf("\n%s size=%lu attrs=0x%08lX\n", Name, Size, Attributes);
  printf("  magic=0x%llX build='%s'\n", Trace->Magic, Trace->Build);
  printf("  last=0x%X %-28s status=0x%X records=%u\n",
         Trace->LastStage, StageName(Trace->LastStage),
         Trace->LastStatus, Trace->RecordCount);

  Count = Trace->RecordCount;
  if (Count > DEBUG_RECORD_COUNT) {
    Count = DEBUG_RECORD_COUNT;
  }

  for (Index = 0; Index < Count; Index++) {
    const DEBUG_RECORD *Record = &Trace->Records[Index];
    printf("  %02u 0x%X %-28s status=0x%llX data=[0x%llX 0x%llX 0x%llX]\n",
           Index, Record->Stage, StageName(Record->Stage),
           Record->Status, Record->Data0, Record->Data1, Record->Data2);
  }
}

static void AcpiScan(void) {
  static const uint8_t GuidBytes[16] = {
      0xde, 0xf8, 0xc9, 0xa0, 0x71, 0x0b, 0xa8, 0x42,
      0xb9, 0x67, 0xe5, 0x38, 0xea, 0xcb, 0x6f, 0x21};
  DWORD Provider = Sig("ACPI");
  DWORD Needed = EnumSystemFirmwareTables(Provider, NULL, 0);
  DWORD *Ids;
  DWORD Count;
  DWORD Index;
  int Hits = 0;

  if (Needed == 0) {
    printf("ACPI scan: failed error=%lu\n", GetLastError());
    return;
  }

  Ids = (DWORD *)HeapAlloc(GetProcessHeap(), 0, Needed);
  if (Ids == NULL) {
    printf("ACPI scan: alloc failed\n");
    return;
  }

  if (EnumSystemFirmwareTables(Provider, Ids, Needed) != Needed) {
    printf("ACPI scan: enum failed error=%lu\n", GetLastError());
    HeapFree(GetProcessHeap(), 0, Ids);
    return;
  }

  Count = Needed / sizeof(DWORD);
  for (Index = 0; Index < Count; Index++) {
    DWORD Size = GetSystemFirmwareTable(Provider, Ids[Index], NULL, 0);
    uint8_t *Table;

    if (Size == 0) {
      continue;
    }

    Table = (uint8_t *)HeapAlloc(GetProcessHeap(), 0, Size);
    if (Table == NULL) {
      continue;
    }

    if (GetSystemFirmwareTable(Provider, Ids[Index], Table, Size) == Size) {
      if (HasBytes(Table, Size, "SMMM", 4) ||
          HasBytes(Table, Size, "MEMDEV", 6) ||
          HasBytes(Table, Size, "WMBD", 4) ||
          HasBytes(Table, Size, GuidBytes, sizeof(GuidBytes))) {
        Hits++;
      }
    }

    HeapFree(GetProcessHeap(), 0, Table);
  }

  printf("ACPI scan: %s\n", Hits ? "found marker" : "no marker");
  HeapFree(GetProcessHeap(), 0, Ids);
}

static void WmiPing(void) {
  HMODULE Advapi;
  WMI_OPEN_BLOCK OpenBlock;
  WMI_EXECUTE_METHOD_W ExecuteMethod;
  WMI_CLOSE_BLOCK CloseBlock;
  HANDLE Block = NULL;
  ULONG Status;
  size_t Index;
  uint8_t In[REQUEST_SIZE];
  uint8_t Out[RESPONSE_SIZE];
  REQUEST *Request = (REQUEST *)In;
  RESPONSE *Response = (RESPONSE *)Out;

  Advapi = LoadLibraryW(L"Advapi32.dll");
  if (Advapi == NULL) {
    printf("Advapi32: load failed\n");
    return;
  }

  OpenBlock = (WMI_OPEN_BLOCK)GetProcAddress(Advapi, "WmiOpenBlock");
  ExecuteMethod =
      (WMI_EXECUTE_METHOD_W)GetProcAddress(Advapi, "WmiExecuteMethodW");
  CloseBlock = (WMI_CLOSE_BLOCK)GetProcAddress(Advapi, "WmiCloseBlock");
  if (OpenBlock == NULL || ExecuteMethod == NULL || CloseBlock == NULL) {
    printf("WMI: exports missing\n");
    FreeLibrary(Advapi);
    return;
  }

  Status = OpenBlock(&gMemGuid, WMIGUID_EXECUTE, &Block);
  printf("WmiOpenBlock: %lu handle=%p\n", Status, Block);
  if (Status != ERROR_SUCCESS) {
    FreeLibrary(Advapi);
    return;
  }

  for (Index = 0; Instances[Index] != NULL; Index++) {
    ULONG OutSize = sizeof(Out);

    memset(In, 0, sizeof(In));
    memset(Out, 0, sizeof(Out));
    Request->Magic = REQ_MAGIC;
    Request->Command = CMD_PING;
    Request->Sequence = 1;

    Status = ExecuteMethod(Block, Instances[Index], WMI_METHOD_ID,
                           REQUEST_SIZE, Request, &OutSize, Out);
    printf("WmiExecuteMethod[%u] \"%ls\" -> %lu out=0x%lX",
           (unsigned)Index,
           Instances[Index][0] ? Instances[Index] : L"<empty>",
           Status, OutSize);

    if (Status == ERROR_SUCCESS) {
      printf(" magic=0x%llX status=0x%X", Response->Magic,
             Response->Status);
    }

    printf("\n");

    if (Status == ERROR_SUCCESS && Response->Magic == RESP_MAGIC) {
      break;
    }
  }

  CloseBlock(Block);
  FreeLibrary(Advapi);
}

int main(void) {
  DEBUG_STATE State;
  DEBUG_TRACE Trace;
  DWORD Attr;
  DWORD Size;

  printf("SmmMem UEFI variable dump\n");

  if (!EnableFirmwareVariablePrivilege()) {
    printf("Run this from Administrator PowerShell.\n");
  }

  Size = ReadVariable(L"SmmMemDebug", &State, sizeof(State), &Attr);
  if (Size != 0) {
    PrintState("SmmMemDebug", &State, Size, Attr);
  }

  Size = ReadVariable(L"SmmMemSmmDebug", &State, sizeof(State), &Attr);
  if (Size != 0) {
    PrintState("SmmMemSmmDebug", &State, Size, Attr);
  }

  Size = ReadVariable(L"SmmMemTrace", &Trace, sizeof(Trace), &Attr);
  if (Size != 0) {
    PrintTrace("SmmMemTrace", &Trace, Size, Attr);
  }

  Size = ReadVariable(L"SmmMemRuntimeTrace", &Trace, sizeof(Trace), &Attr);
  if (Size != 0) {
    PrintTrace("SmmMemRuntimeTrace", &Trace, Size, Attr);
  }

  printf("\nSmmMem ACPI/WMI check\n");
  AcpiScan();
  WmiPing();

  return 0;
}
