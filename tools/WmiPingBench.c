#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#define WMIGUID_EXECUTE 0x0010U
#define WMI_METHOD_ID 1U
#define REQUEST_SIZE 4096U
#define RESPONSE_SIZE 512U
#define REQ_MAGIC 0x5145524D4D5355ULL
#define RESP_MAGIC 0x5345524D4D5355ULL
#define CMD_PING 1U

typedef ULONG(WINAPI *WMI_OPEN_BLOCK)(GUID *, DWORD, HANDLE *);
typedef ULONG(WINAPI *WMI_EXECUTE_METHOD_W)(HANDLE, const wchar_t *, ULONG,
                                            ULONG, void *, ULONG *, void *);
typedef ULONG(WINAPI *WMI_CLOSE_BLOCK)(HANDLE);

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
  uint8_t Data[352];
} RESPONSE;
#pragma pack(pop)

static const GUID gGuid = {
    0xa0c9f8de, 0x0b71, 0x42a8,
    {0xb9, 0x67, 0xe5, 0x38, 0xea, 0xcb, 0x6f, 0x21}};

static const wchar_t *gInstances[] = {
    L"ACPI\\PNP0C14\\Mem_0", L"ACPI\\PNP0C14\\SMMM_0",
    L"ACPI\\PNP0C14\\0_0", L"", NULL};

static uint64_t NowTicks(void) {
  LARGE_INTEGER Value;
  QueryPerformanceCounter(&Value);
  return (uint64_t)Value.QuadPart;
}

static double TicksToMicroseconds(uint64_t Ticks, LARGE_INTEGER Frequency) {
  return (double)Ticks * 1000000.0 / (double)Frequency.QuadPart;
}

static int ParseUnsigned(const wchar_t *Text, unsigned long *Value,
                         int AllowZero) {
  wchar_t *End;
  unsigned long Parsed;
  if (Text == NULL || Text[0] == L'-' || Text[0] == L'\0') {
    return 0;
  }
  Parsed = wcstoul(Text, &End, 10);
  if (*End != L'\0' || (!AllowZero && Parsed == 0) ||
      Parsed > 1000000UL) {
    return 0;
  }
  *Value = Parsed;
  return 1;
}

int wmain(int argc, wchar_t **argv) {
  HMODULE Advapi;
  WMI_OPEN_BLOCK OpenBlock;
  WMI_EXECUTE_METHOD_W ExecuteMethod;
  WMI_CLOSE_BLOCK CloseBlock;
  HANDLE Block = NULL;
  LARGE_INTEGER Frequency;
  unsigned long Iterations = 30;
  unsigned long DelayMs = 0;
  unsigned long Index;
  uint64_t Sequence = 0;
  uint8_t Input[REQUEST_SIZE];
  uint8_t Output[RESPONSE_SIZE];
  REQUEST *Request = (REQUEST *)Input;
  RESPONSE *Response = (RESPONSE *)Output;
  int Failed = 0;

  if (argc > 3 ||
      (argc > 1 && !ParseUnsigned(argv[1], &Iterations, 0)) ||
      (argc > 2 && !ParseUnsigned(argv[2], &DelayMs, 1))) {
    fwprintf(stderr, L"Usage: WmiPingBench.exe [iterations] [delay-ms]\n");
    return 2;
  }
  Advapi = LoadLibraryW(L"Advapi32.dll");
  if (Advapi == NULL) {
    fwprintf(stderr, L"LoadLibrary(Advapi32.dll) failed: %lu\n", GetLastError());
    return 1;
  }
  OpenBlock = (WMI_OPEN_BLOCK)GetProcAddress(Advapi, "WmiOpenBlock");
  ExecuteMethod = (WMI_EXECUTE_METHOD_W)GetProcAddress(
      Advapi, "WmiExecuteMethodW");
  CloseBlock = (WMI_CLOSE_BLOCK)GetProcAddress(Advapi, "WmiCloseBlock");
  if (OpenBlock == NULL || ExecuteMethod == NULL || CloseBlock == NULL) {
    fwprintf(stderr, L"Required WMI exports are missing.\n");
    FreeLibrary(Advapi);
    return 1;
  }
  if (OpenBlock((GUID *)&gGuid, WMIGUID_EXECUTE, &Block) != ERROR_SUCCESS) {
    fwprintf(stderr, L"WmiOpenBlock failed.\n");
    FreeLibrary(Advapi);
    return 1;
  }
  if (!QueryPerformanceFrequency(&Frequency)) {
    fwprintf(stderr, L"QueryPerformanceFrequency failed.\n");
    CloseBlock(Block);
    FreeLibrary(Advapi);
    return 1;
  }

  wprintf(L"protocol,command,request_size,response_capacity,iteration,instance,"
          L"elapsed_us,wmi_status,response_status\n");
  for (Index = 0; Index < Iterations; Index++) {
    ULONG OutSize = sizeof(Output);
    ULONG WmiStatus = ERROR_NOT_FOUND;
    uint64_t Start;
    uint64_t End;
    const wchar_t *UsedInstance = L"<none>";

    ZeroMemory(Input, sizeof(Input));
    ZeroMemory(Output, sizeof(Output));
    Request->Magic = REQ_MAGIC;
    Request->Command = CMD_PING;
    Request->Sequence = ++Sequence;
    Start = NowTicks();
    for (size_t Instance = 0; gInstances[Instance] != NULL; Instance++) {
      WmiStatus = ExecuteMethod(Block, gInstances[Instance], WMI_METHOD_ID,
                                 REQUEST_SIZE, Request, &OutSize, Output);
      if (WmiStatus == ERROR_SUCCESS) {
        UsedInstance = gInstances[Instance];
        break;
      }
      OutSize = sizeof(Output);
    }
    End = NowTicks();
    wprintf(L"src-direct-wmi,CMD_PING,%u,%u,%lu,%ls,%.2f,0x%08lX,0x%08X\n",
            REQUEST_SIZE, RESPONSE_SIZE, Index + 1, UsedInstance,
            TicksToMicroseconds(End - Start, Frequency), WmiStatus,
            (WmiStatus == ERROR_SUCCESS && OutSize >= 16 &&
                     Response->Magic == RESP_MAGIC
                 ? Response->Status
                 : 0xFFFFFFFFU));
    if (WmiStatus != ERROR_SUCCESS || OutSize < 16 ||
        Response->Magic != RESP_MAGIC || Response->Status != 0) {
      Failed = 1;
      break;
    }
    if (DelayMs != 0 && Index + 1 < Iterations) {
      Sleep(DelayMs);
    }
  }

  CloseBlock(Block);
  FreeLibrary(Advapi);
  return Failed ? 1 : 0;
}
