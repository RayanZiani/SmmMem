#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include "Api.h"

#define WMIGUID_EXECUTE 0x0010
#define WMI_METHOD_ID 1U
#define REQUEST_SIZE 4096U
#define RESPONSE_SIZE 512U
#define RESPONSE_DATA_SIZE 352U
#define REQ_MAGIC 0x5145524D4D5355ULL
#define RESP_MAGIC 0x5345524D4D5355ULL
#define STATUS_OK 0U

#define CMD_PING 1U
#define CMD_READ_PHYS 2U
#define CMD_WRITE_PHYS 3U
#define CMD_TRANSLATE_VIRT 4U
#define CMD_READ_VIRT 5U
#define CMD_WRITE_VIRT 6U
#define CMD_FIND_PROCESS_PID 7U
#define CMD_FIND_PROCESS_NAME 8U
#define CMD_FIND_MODULE 9U
#define CMD_FIND_KERNEL_MODULE 10U
#define CMD_FIND_EXPORT 11U

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

#pragma pack(pop)

typedef struct {
  HMODULE Advapi;
  WMI_OPEN_BLOCK OpenBlock;
  WMI_EXECUTE_METHOD_W ExecuteMethod;
  WMI_CLOSE_BLOCK CloseBlock;
  HANDLE Block;
  const wchar_t **Instances;
  uint64_t Sequence;
} STATE;

static GUID gMemGuid = {
    0xa0c9f8de,
    0x0b71,
    0x42a8,
    {0xb9, 0x67, 0xe5, 0x38, 0xea, 0xcb, 0x6f, 0x21}};
static const wchar_t *gMemInstances[] = {
    L"ACPI\\PNP0C14\\Mem_0",
    L"ACPI\\PNP0C14\\SMMM_0",
    L"ACPI\\PNP0C14\\0_0",
    L"",
    NULL};

static STATE g;

static void CloseRaw(void) {
  if (g.CloseBlock != NULL && g.Block != NULL) {
    g.CloseBlock(g.Block);
  }
  if (g.Advapi != NULL) {
    FreeLibrary(g.Advapi);
  }
  ZeroMemory(&g, sizeof(g));
}

static int OpenRaw(GUID *Guid, const wchar_t **Instances) {
  ULONG Status;

  CloseRaw();
  g.Advapi = LoadLibraryW(L"Advapi32.dll");
  if (g.Advapi == NULL) {
    return 0;
  }
  g.OpenBlock = (WMI_OPEN_BLOCK)GetProcAddress(g.Advapi, "WmiOpenBlock");
  g.ExecuteMethod =
      (WMI_EXECUTE_METHOD_W)GetProcAddress(g.Advapi, "WmiExecuteMethodW");
  g.CloseBlock = (WMI_CLOSE_BLOCK)GetProcAddress(g.Advapi, "WmiCloseBlock");
  if (g.OpenBlock == NULL || g.ExecuteMethod == NULL ||
      g.CloseBlock == NULL) {
    CloseRaw();
    return 0;
  }
  Status = g.OpenBlock(Guid, WMIGUID_EXECUTE, &g.Block);
  if (Status != ERROR_SUCCESS) {
    CloseRaw();
    return 0;
  }
  g.Instances = Instances;
  g.Sequence = GetTickCount64();
  return 1;
}

int Init(void) {
  return OpenRaw(&gMemGuid, gMemInstances);
}

void Close(void) {
  CloseRaw();
}

static void InitRequest(REQUEST *Request, uint32_t Command) {
  ZeroMemory(Request, REQUEST_SIZE);
  Request->Magic = REQ_MAGIC;
  Request->Command = Command;
  Request->Sequence = ++g.Sequence;
}

static int ExecuteStandalone(REQUEST *Request, RESPONSE *Response) {
  uint8_t Out[RESPONSE_SIZE];
  ULONG OutSize;
  ULONG Status;

  for (size_t Index = 0; g.Instances[Index] != NULL; Index++) {
    ZeroMemory(Out, sizeof(Out));
    OutSize = sizeof(Out);
    Status = g.ExecuteMethod(g.Block, g.Instances[Index], WMI_METHOD_ID,
                             REQUEST_SIZE, Request, &OutSize, Out);
    if (Status == ERROR_SUCCESS) {
      CopyMemory(Response, Out, sizeof(*Response));
      return Response->Magic == RESP_MAGIC;
    }
  }
  return 0;
}

static int Send(REQUEST *Request, RESPONSE *Response) {
  if (g.Block == NULL) {
    return 0;
  }
  ZeroMemory(Response, sizeof(*Response));
  return ExecuteStandalone(Request, Response);
}

int Ping(void) {
  uint8_t In[REQUEST_SIZE];
  REQUEST *Request = (REQUEST *)In;
  RESPONSE Response;

  InitRequest(Request, CMD_PING);
  return Send(Request, &Response) && Response.Status == STATUS_OK;
}

int FindProcessByPid(uint32_t Pid, PROCESS_INFO *Process) {
  uint8_t In[REQUEST_SIZE];
  REQUEST *Request = (REQUEST *)In;
  RESPONSE Response;

  InitRequest(Request, CMD_FIND_PROCESS_PID);
  Request->Arg1 = Pid;
  if (!Send(Request, &Response) || Response.Status != STATUS_OK ||
      Response.DataSize < sizeof(*Process)) {
    return 0;
  }
  CopyMemory(Process, Response.Data, sizeof(*Process));
  return 1;
}

int FindProcessByName(const char *Name, PROCESS_INFO *Process) {
  uint8_t In[REQUEST_SIZE];
  REQUEST *Request = (REQUEST *)In;
  RESPONSE Response;
  size_t Size = strlen(Name) + 1;

  if (Size > RESPONSE_DATA_SIZE) {
    return 0;
  }
  InitRequest(Request, CMD_FIND_PROCESS_NAME);
  Request->DataSize = (uint32_t)Size;
  CopyMemory(Request->Data, Name, Size);
  if (!Send(Request, &Response) || Response.Status != STATUS_OK ||
      Response.DataSize < sizeof(*Process)) {
    return 0;
  }
  CopyMemory(Process, Response.Data, sizeof(*Process));
  return 1;
}

int TranslateVirt(uint32_t Pid, uint64_t Va, uint64_t *Pa) {
  uint8_t In[REQUEST_SIZE];
  REQUEST *Request = (REQUEST *)In;
  RESPONSE Response;

  InitRequest(Request, CMD_TRANSLATE_VIRT);
  Request->Arg1 = Pid;
  Request->Arg2 = Va;
  if (!Send(Request, &Response) || Response.Status != STATUS_OK) {
    return 0;
  }
  *Pa = Response.Result;
  return 1;
}

int ReadPhys(uint64_t Address, void *Buffer, uint32_t Size) {
  uint8_t In[REQUEST_SIZE];
  REQUEST *Request = (REQUEST *)In;
  RESPONSE Response;
  uint32_t Done = 0;

  while (Done < Size) {
    uint32_t Chunk = Size - Done;
    if (Chunk > RESPONSE_DATA_SIZE) {
      Chunk = RESPONSE_DATA_SIZE;
    }
    InitRequest(Request, CMD_READ_PHYS);
    Request->Arg1 = Address + Done;
    Request->Arg2 = Chunk;
    if (!Send(Request, &Response) || Response.Status != STATUS_OK ||
        Response.DataSize != Chunk) {
      return 0;
    }
    CopyMemory((uint8_t *)Buffer + Done, Response.Data, Chunk);
    Done += Chunk;
  }
  return 1;
}

int WritePhys(uint64_t Address, const void *Buffer, uint32_t Size) {
  uint8_t In[REQUEST_SIZE];
  REQUEST *Request = (REQUEST *)In;
  RESPONSE Response;
  uint32_t Done = 0;

  while (Done < Size) {
    uint32_t Chunk = Size - Done;
    if (Chunk > RESPONSE_DATA_SIZE) {
      Chunk = RESPONSE_DATA_SIZE;
    }
    InitRequest(Request, CMD_WRITE_PHYS);
    Request->Arg1 = Address + Done;
    Request->DataSize = Chunk;
    CopyMemory(Request->Data, (const uint8_t *)Buffer + Done, Chunk);
    if (!Send(Request, &Response) || Response.Status != STATUS_OK) {
      return 0;
    }
    Done += Chunk;
  }
  return 1;
}

int ReadVirt(uint32_t Pid, uint64_t Address, void *Buffer, uint32_t Size) {
  uint8_t In[REQUEST_SIZE];
  REQUEST *Request = (REQUEST *)In;
  RESPONSE Response;
  uint32_t Done = 0;

  while (Done < Size) {
    uint32_t Chunk = Size - Done;
    if (Chunk > RESPONSE_DATA_SIZE) {
      Chunk = RESPONSE_DATA_SIZE;
    }
    InitRequest(Request, CMD_READ_VIRT);
    Request->Arg1 = Pid;
    Request->Arg2 = Address + Done;
    Request->Arg3 = Chunk;
    if (!Send(Request, &Response) || Response.Status != STATUS_OK ||
        Response.DataSize != Chunk) {
      return 0;
    }
    CopyMemory((uint8_t *)Buffer + Done, Response.Data, Chunk);
    Done += Chunk;
  }
  return 1;
}

int WriteVirt(uint32_t Pid, uint64_t Address, const void *Buffer,
              uint32_t Size) {
  uint8_t In[REQUEST_SIZE];
  REQUEST *Request = (REQUEST *)In;
  RESPONSE Response;
  uint32_t Done = 0;

  while (Done < Size) {
    uint32_t Chunk = Size - Done;
    if (Chunk > RESPONSE_DATA_SIZE) {
      Chunk = RESPONSE_DATA_SIZE;
    }
    InitRequest(Request, CMD_WRITE_VIRT);
    Request->Arg1 = Pid;
    Request->Arg2 = Address + Done;
    Request->DataSize = Chunk;
    CopyMemory(Request->Data, (const uint8_t *)Buffer + Done, Chunk);
    if (!Send(Request, &Response) || Response.Status != STATUS_OK) {
      return 0;
    }
    Done += Chunk;
  }
  return 1;
}

int FindModule(const PROCESS_INFO *Process, const char *Name,
               MODULE_INFO *Module) {
  uint8_t In[REQUEST_SIZE];
  REQUEST *Request = (REQUEST *)In;
  RESPONSE Response;
  size_t Size = strlen(Name) + 1;

  if (Size > RESPONSE_DATA_SIZE) {
    return 0;
  }
  InitRequest(Request, CMD_FIND_MODULE);
  Request->Arg1 = Process->Pid;
  Request->DataSize = (uint32_t)Size;
  CopyMemory(Request->Data, Name, Size);
  if (!Send(Request, &Response) || Response.Status != STATUS_OK ||
      Response.DataSize < sizeof(*Module)) {
    return 0;
  }
  CopyMemory(Module, Response.Data, sizeof(*Module));
  return 1;
}

int FindKernelModule(const char *Name, MODULE_INFO *Module) {
  uint8_t In[REQUEST_SIZE];
  REQUEST *Request = (REQUEST *)In;
  RESPONSE Response;
  size_t Size = strlen(Name) + 1;

  if (Size > RESPONSE_DATA_SIZE) {
    return 0;
  }
  InitRequest(Request, CMD_FIND_KERNEL_MODULE);
  Request->DataSize = (uint32_t)Size;
  CopyMemory(Request->Data, Name, Size);
  if (!Send(Request, &Response) || Response.Status != STATUS_OK ||
      Response.DataSize < sizeof(*Module)) {
    return 0;
  }
  CopyMemory(Module, Response.Data, sizeof(*Module));
  return 1;
}

int FindExport(const MODULE_INFO *Module, const char *Name,
               uint64_t *Address) {
  uint8_t In[REQUEST_SIZE];
  REQUEST *Request = (REQUEST *)In;
  RESPONSE Response;
  size_t NameSize = strlen(Name) + 1;
  uint32_t Size = (uint32_t)(sizeof(*Module) + NameSize);

  if (Size > RESPONSE_DATA_SIZE) {
    return 0;
  }
  InitRequest(Request, CMD_FIND_EXPORT);
  Request->DataSize = Size;
  CopyMemory(Request->Data, Module, sizeof(*Module));
  CopyMemory(Request->Data + sizeof(*Module), Name, NameSize);
  if (!Send(Request, &Response) || Response.Status != STATUS_OK) {
    return 0;
  }
  *Address = Response.Result;
  return 1;
}

int Dump(const MODULE_INFO *Module, DUMP_CALLBACK Callback, void *Context) {
  uint8_t Buffer[RESPONSE_DATA_SIZE];
  uint64_t Done = 0;

  if (Module == NULL || Module->Pid == 0 || Module->Base == 0 ||
      Module->Size == 0) {
    return 0;
  }
  while (Done < Module->Size) {
    uint64_t Address = Module->Base + Done;
    uint32_t Chunk = (uint32_t)(Module->Size - Done);
    if (Chunk > sizeof(Buffer)) {
      Chunk = sizeof(Buffer);
    }
    if (Chunk > 0x1000U - (uint32_t)(Address & 0xFFFU)) {
      Chunk = 0x1000U - (uint32_t)(Address & 0xFFFU);
    }
    if (!ReadVirt(Module->Pid, Address, Buffer, Chunk)) {
      ZeroMemory(Buffer, Chunk);
    }
    if (Callback != NULL && !Callback(Address, Buffer, Chunk, Context)) {
      return 0;
    }
    Done += Chunk;
  }
  return 1;
}

#ifndef API_ONLY
int wmain(int argc, wchar_t **argv) {
  if (argc == 1 || (argc == 2 && _wcsicmp(argv[1], L"ping") == 0)) {
    int Ok = Init() && Ping();
    printf("%s\n", Ok ? "pong" : "failed");
    Close();
    return Ok ? 0 : 1;
  }
  printf("Usage: mem-client.exe [ping]\n");
  return 1;
}
#endif