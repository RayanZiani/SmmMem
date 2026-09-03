#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define WMIGUID_EXECUTE 0x0010
#define WMI_METHOD_ID 1U

#define WMI_REQUEST_MAGIC 0x00514552574D4D53ULL
#define WMI_RESPONSE_MAGIC 0x00534552574D4D53ULL
#define WMI_REQUEST_SIZE 4096U
#define WMI_RESPONSE_SIZE 512U
#define WMI_COMMAND_DOORBELL 1U
#define WMI_COMMAND_PING 2U
#define WMI_COMMAND_STATUS 3U
#define WMI_COMMAND_UNLOAD 4U
#define WMI_COMMAND_STAGE_CHUNK 5U
#define WMI_COMMAND_RELOAD 6U
#define WMI_STATUS_OK 0U

typedef ULONG(WINAPI *WMI_OPEN_BLOCK)(GUID *Guid, DWORD DesiredAccess,
                                      HANDLE *DataBlockHandle);
typedef ULONG(WINAPI *WMI_EXECUTE_METHOD_W)(HANDLE DataBlockHandle,
                                            const wchar_t *InstanceName,
                                            ULONG MethodId,
                                            ULONG InBufferSize,
                                            void *InBuffer,
                                            ULONG *OutBufferSize,
                                            void *OutBuffer);
typedef ULONG(WINAPI *WMI_CLOSE_BLOCK)(HANDLE DataBlockHandle);

#pragma pack(push, 1)
typedef struct {
  uint64_t Magic;
  uint32_t Command;
  uint32_t DataSize;
  uint64_t Offset;
  uint64_t PayloadSize;
  uint64_t PayloadHash;
  uint64_t Sequence;
  uint8_t Data[1];
} WMI_REQUEST;

typedef struct {
  uint64_t Magic;
  uint32_t Size;
  uint32_t Status;
  uint32_t Loaded;
  uint32_t Generation;
  uint32_t LastCommand;
  uint32_t DebugLogSize;
  uint64_t Result;
  uint64_t Sequence;
  uint8_t DebugLog[464];
} WMI_RESPONSE;
#pragma pack(pop)

#define WMI_REQUEST_DATA_CAPACITY 4000U

typedef struct {
  HMODULE Advapi;
  WMI_OPEN_BLOCK OpenBlock;
  WMI_EXECUTE_METHOD_W ExecuteMethod;
  WMI_CLOSE_BLOCK CloseBlock;
  HANDLE Block;
  const wchar_t *Instance;
} WMI_CLIENT;

static GUID gDoorbellGuid = {
    0x9b6f1a20,
    0x31d5,
    0x44df,
    {0x9a, 0x9c, 0x15, 0x7f, 0x43, 0x07, 0x91, 0x4b}};

static const wchar_t *gInstances[] = {
    L"ACPI\\PNP0C14\\SmmMapper_0",
    L"ACPI\\PNP0C14\\SMMP_0",
    L"ACPI\\PNP0C14\\0_0",
    L"",
    NULL};

static uint64_t FnV1a64(const void *Buffer, uint32_t Size) {
  const unsigned char *Data = (const unsigned char *)Buffer;
  uint64_t Hash = 0xCBF29CE484222325ULL;
  while (Size--) {
    Hash ^= *Data++;
    Hash *= 0x100000001B3ULL;
  }
  return Hash;
}

static uint8_t *ReadFileBytes(const wchar_t *Path, DWORD *SizeOut) {
  HANDLE File;
  LARGE_INTEGER Size;
  DWORD Got;
  uint8_t *Bytes;

  *SizeOut = 0;
  File = CreateFileW(Path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                     FILE_ATTRIBUTE_NORMAL, NULL);
  if (File == INVALID_HANDLE_VALUE) {
    return NULL;
  }
  if (!GetFileSizeEx(File, &Size) || Size.QuadPart <= 0 ||
      Size.QuadPart > 256 * 1024) {
    CloseHandle(File);
    return NULL;
  }
  Bytes = (uint8_t *)HeapAlloc(GetProcessHeap(), 0, (SIZE_T)Size.QuadPart);
  if (Bytes == NULL) {
    CloseHandle(File);
    return NULL;
  }
  if (!ReadFile(File, Bytes, (DWORD)Size.QuadPart, &Got, NULL) ||
      Got != (DWORD)Size.QuadPart) {
    HeapFree(GetProcessHeap(), 0, Bytes);
    CloseHandle(File);
    return NULL;
  }
  CloseHandle(File);
  *SizeOut = Got;
  return Bytes;
}

static void PrintResponse(const WMI_RESPONSE *Response) {
  DWORD LogSize;

  if (Response == NULL || Response->Magic != WMI_RESPONSE_MAGIC) {
    fwprintf(stderr, L"Invalid or missing SMM response.\n");
    return;
  }
  wprintf(L"status=0x%08X result=0x%llX loaded=%u generation=%u "
          L"sequence=%llu\n",
          Response->Status, (unsigned long long)Response->Result,
          Response->Loaded, Response->Generation,
          (unsigned long long)Response->Sequence);
  LogSize = Response->DebugLogSize;
  if (LogSize > sizeof(Response->DebugLog)) {
    LogSize = sizeof(Response->DebugLog);
  }
  if (LogSize != 0) {
    fwrite(Response->DebugLog, 1, LogSize, stdout);
    if (Response->DebugLog[LogSize - 1] != '\n') {
      fputc('\n', stdout);
    }
  }
}

static int OpenWmi(WMI_CLIENT *Client) {
  ULONG Status;

  ZeroMemory(Client, sizeof(*Client));
  Client->Advapi = LoadLibraryW(L"Advapi32.dll");
  if (Client->Advapi == NULL) {
    fwprintf(stderr, L"LoadLibrary(Advapi32) failed: %lu\n", GetLastError());
    return 0;
  }
  Client->OpenBlock =
      (WMI_OPEN_BLOCK)GetProcAddress(Client->Advapi, "WmiOpenBlock");
  Client->ExecuteMethod =
      (WMI_EXECUTE_METHOD_W)GetProcAddress(Client->Advapi,
                                           "WmiExecuteMethodW");
  Client->CloseBlock =
      (WMI_CLOSE_BLOCK)GetProcAddress(Client->Advapi, "WmiCloseBlock");
  if (Client->OpenBlock == NULL || Client->ExecuteMethod == NULL) {
    fwprintf(stderr, L"Required WMI exports are missing.\n");
    FreeLibrary(Client->Advapi);
    return 0;
  }
  Status = Client->OpenBlock(&gDoorbellGuid, WMIGUID_EXECUTE,
                             &Client->Block);
  if (Status != ERROR_SUCCESS) {
    fwprintf(stderr, L"WmiOpenBlock failed: 0x%08lX\n", Status);
    FreeLibrary(Client->Advapi);
    return 0;
  }
  return 1;
}

static void CloseWmi(WMI_CLIENT *Client) {
  if (Client->CloseBlock != NULL && Client->Block != NULL) {
    Client->CloseBlock(Client->Block);
  }
  if (Client->Advapi != NULL) {
    FreeLibrary(Client->Advapi);
  }
}

static int ExecuteWmi(WMI_CLIENT *Client, WMI_REQUEST *Request,
                      WMI_RESPONSE *Response) {
  ULONG OutSize;
  ULONG Status;

  for (size_t Index = 0; gInstances[Index] != NULL; Index++) {
    ZeroMemory(Response, sizeof(*Response));
    OutSize = sizeof(*Response);
    Status = Client->ExecuteMethod(Client->Block, gInstances[Index],
                                   WMI_METHOD_ID, WMI_REQUEST_SIZE, Request,
                                   &OutSize, Response);
    if (Status == ERROR_SUCCESS) {
      Client->Instance = gInstances[Index];
      if (OutSize >= 16 && Response->Magic == WMI_RESPONSE_MAGIC) {
        return 1;
      }
      fwprintf(stderr, L"WMI method returned no SMM response.\n");
      return 0;
    }
  }
  fwprintf(stderr, L"Could not execute the ACPI WMI method.\n");
  return 0;
}

static void InitRequest(WMI_REQUEST *Request, uint32_t Command) {
  ZeroMemory(Request, WMI_REQUEST_SIZE);
  Request->Magic = WMI_REQUEST_MAGIC;
  Request->Command = Command;
}

static int SendSimpleCommand(uint32_t Command) {
  WMI_CLIENT Client;
  WMI_REQUEST *Request;
  WMI_RESPONSE Response;
  int Ok;

  Request = (WMI_REQUEST *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                     WMI_REQUEST_SIZE);
  if (Request == NULL) {
    return 1;
  }
  InitRequest(Request, Command);
  if (!OpenWmi(&Client)) {
    HeapFree(GetProcessHeap(), 0, Request);
    return 1;
  }
  Ok = ExecuteWmi(&Client, Request, &Response);
  if (Ok) {
    wprintf(L"instance=%ls\n", Client.Instance);
    PrintResponse(&Response);
  }
  CloseWmi(&Client);
  HeapFree(GetProcessHeap(), 0, Request);
  return Ok && Response.Status == WMI_STATUS_OK ? 0 : 1;
}

static int ReloadPayload(const wchar_t *PayloadPath) {
  WMI_CLIENT Client;
  WMI_REQUEST *Request;
  WMI_RESPONSE Response;
  uint8_t *Payload;
  DWORD PayloadSize;
  DWORD Offset;
  uint64_t Hash;
  int Ok = 1;

  Payload = ReadFileBytes(PayloadPath, &PayloadSize);
  if (Payload == NULL) {
    fwprintf(stderr, L"Could not read payload %ls\n", PayloadPath);
    return 1;
  }
  Request = (WMI_REQUEST *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                     WMI_REQUEST_SIZE);
  if (Request == NULL) {
    HeapFree(GetProcessHeap(), 0, Payload);
    return 1;
  }
  if (!OpenWmi(&Client)) {
    HeapFree(GetProcessHeap(), 0, Payload);
    HeapFree(GetProcessHeap(), 0, Request);
    return 1;
  }

  Hash = FnV1a64(Payload, PayloadSize);
  wprintf(L"payload size=0x%X hash=0x%llX\n", PayloadSize,
          (unsigned long long)Hash);
  for (Offset = 0; Offset < PayloadSize; Offset += WMI_REQUEST_DATA_CAPACITY) {
    DWORD Chunk = PayloadSize - Offset;
    if (Chunk > WMI_REQUEST_DATA_CAPACITY) {
      Chunk = WMI_REQUEST_DATA_CAPACITY;
    }
    InitRequest(Request, WMI_COMMAND_STAGE_CHUNK);
    Request->DataSize = Chunk;
    Request->Offset = Offset;
    Request->PayloadSize = PayloadSize;
    Request->PayloadHash = Hash;
    CopyMemory(Request->Data, Payload + Offset, Chunk);
    if (!ExecuteWmi(&Client, Request, &Response) ||
        Response.Status != WMI_STATUS_OK) {
      fwprintf(stderr, L"stage failed at offset 0x%X\n", Offset);
      PrintResponse(&Response);
      Ok = 0;
      goto Done;
    }
  }

  InitRequest(Request, WMI_COMMAND_RELOAD);
  Request->PayloadSize = PayloadSize;
  Request->PayloadHash = Hash;
  if (!ExecuteWmi(&Client, Request, &Response)) {
    Ok = 0;
    goto Done;
  }
  wprintf(L"instance=%ls\n", Client.Instance);
  PrintResponse(&Response);
  Ok = Response.Status == WMI_STATUS_OK;

Done:
  CloseWmi(&Client);
  HeapFree(GetProcessHeap(), 0, Payload);
  HeapFree(GetProcessHeap(), 0, Request);
  return Ok ? 0 : 1;
}

int wmain(int argc, wchar_t **argv) {
  if (argc == 2 && _wcsicmp(argv[1], L"doorbell") == 0) {
    return SendSimpleCommand(WMI_COMMAND_DOORBELL);
  }
  if (argc == 2 && _wcsicmp(argv[1], L"status") == 0) {
    return SendSimpleCommand(WMI_COMMAND_STATUS);
  }
  if (argc == 2 && _wcsicmp(argv[1], L"ping") == 0) {
    return SendSimpleCommand(WMI_COMMAND_PING);
  }
  if (argc == 2 && _wcsicmp(argv[1], L"unload") == 0) {
    return SendSimpleCommand(WMI_COMMAND_UNLOAD);
  }
  if (argc == 3 && _wcsicmp(argv[1], L"reload") == 0) {
    return ReloadPayload(argv[2]);
  }

  fwprintf(stderr,
           L"Usage:\n"
           L"  SmmClient.exe doorbell\n"
           L"  SmmClient.exe status\n"
           L"  SmmClient.exe ping\n"
           L"  SmmClient.exe unload\n"
           L"  SmmClient.exe reload C:\\path\\PAYLOAD.EFI\n");
  return 1;
}