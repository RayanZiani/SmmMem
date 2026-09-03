#include <intrin.h>

#pragma intrinsic(__inbyte)
#pragma intrinsic(__outbyte)
#pragma function(memset)
#pragma function(memcpy)

typedef unsigned char UINT8;
typedef unsigned char BOOLEAN;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef unsigned long long UINT64;
typedef unsigned long long UINTN;
typedef long long INTN;
typedef unsigned long long size_t;
typedef unsigned short CHAR16;
typedef void VOID;
typedef VOID *EFI_HANDLE;
typedef VOID *EFI_EVENT;
typedef UINT64 EFI_STATUS;
typedef UINTN EFI_TPL;

#define EFIAPI __cdecl

#define EFI_SUCCESS 0
#define EFI_LOAD_ERROR 1
#define EFI_INVALID_PARAMETER 2
#define EFI_UNSUPPORTED 3
#define EFI_BUFFER_TOO_SMALL 5
#define EFI_NOT_FOUND 14
#define EFI_ERROR(Status) ((Status) != EFI_SUCCESS)
#define OFFSET_OF(Type, Field) ((UINTN)&(((Type *)0)->Field))

#define EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL 0x00000001U
#define EVT_NOTIFY_SIGNAL 0x00000200U
#define TPL_CALLBACK 8U
#define EfiBootServicesData 4

#define COM1_PORT 0x3F8
#define PAYLOAD_FILE_LIMIT (256U * 1024U)
#define PAYLOAD_IMAGE_LIMIT (768U * 1024U)
#define COMM_MAGIC 0x56444D53U
#define COMM_LOAD_INLINE 1U
#define COMM_LOAD_MAILBOX 2U
#define COMM_HEADER_SIZE 32U
#define REASON_LOAD 1U
#define REASON_UNLOAD 2U
#define REASON_DOORBELL 3U
#define MAILBOX_MAGIC 0x58425645444D4D53ULL
#define MAILBOX_HEADER_SIZE 0x1000U
#define MAILBOX_PAYLOAD_CAPACITY (256U * 1024U)
#define MAILBOX_LOG_CAPACITY 3072U
#define MAILBOX_TOTAL_SIZE \
  (MAILBOX_HEADER_SIZE + MAILBOX_PAYLOAD_CAPACITY)
#define SW_SMI_VALUE 0xD5U
#define WMI_REQUEST_MAGIC 0x00514552574D4D53ULL
#define WMI_RESPONSE_MAGIC 0x00534552574D4D53ULL
#define WMI_REQUEST_SIZE 4096U
#define WMI_RESPONSE_OFFSET 0xD00U
#define WMI_RESPONSE_SIZE 512U
#define WMI_COMMAND_DOORBELL 1U
#define WMI_COMMAND_PING 2U
#define WMI_COMMAND_STATUS 3U
#define WMI_COMMAND_UNLOAD 4U
#define WMI_COMMAND_STAGE_CHUNK 5U
#define WMI_COMMAND_RELOAD 6U
#define MAX_TRACKED_HANDLERS 16U
#define MAX_TRACKED_POOLS 16U
#define EfiRuntimeServicesData 6U

typedef struct {
  UINT32 Data1;
  UINT16 Data2;
  UINT16 Data3;
  UINT8 Data4[8];
} EFI_GUID;

typedef struct {
  UINT32 Magic;
  UINT32 Command;
  UINT32 PayloadSize;
  UINT64 MailboxPhysical;
  UINT32 MailboxSize;
  UINT32 SwSmiValue;
  UINT8 Payload[1];
} COMM_MESSAGE;

typedef struct {
  UINT64 Signature;
  UINT32 Revision;
  UINT32 HeaderSize;
  UINT32 CRC32;
  UINT32 Reserved;
} EFI_TABLE_HEADER;

typedef struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;
typedef struct EFI_FILE_PROTOCOL EFI_FILE_PROTOCOL;
typedef struct EFI_BOOT_SERVICES EFI_BOOT_SERVICES;
typedef struct EFI_SYSTEM_TABLE EFI_SYSTEM_TABLE;

typedef EFI_STATUS(EFIAPI *EFI_LOCATE_HANDLE_BUFFER)(
    UINTN SearchType, EFI_GUID *Protocol, VOID *SearchKey, UINTN *NoHandles,
    EFI_HANDLE **Buffer);
typedef EFI_STATUS(EFIAPI *EFI_LOCATE_PROTOCOL)(EFI_GUID *Protocol,
                                                VOID *Registration,
                                                VOID **Interface);
typedef EFI_STATUS(EFIAPI *EFI_HANDLE_PROTOCOL)(EFI_HANDLE Handle,
                                                EFI_GUID *Protocol,
                                                VOID **Interface);
typedef EFI_STATUS(EFIAPI *EFI_FREE_POOL)(VOID *Buffer);
typedef VOID(EFIAPI *EFI_EVENT_NOTIFY)(EFI_EVENT Event, VOID *Context);
typedef EFI_STATUS(EFIAPI *EFI_CREATE_EVENT)(UINT32 Type, EFI_TPL NotifyTpl,
                                             EFI_EVENT_NOTIFY NotifyFunction,
                                             VOID *NotifyContext,
                                             EFI_EVENT *Event);
typedef EFI_STATUS(EFIAPI *EFI_CLOSE_EVENT)(EFI_EVENT Event);
typedef EFI_STATUS(EFIAPI *EFI_REGISTER_PROTOCOL_NOTIFY)(EFI_GUID *Protocol,
                                                         EFI_EVENT Event,
                                                         VOID **Registration);
typedef struct EFI_SMM_BASE2_PROTOCOL EFI_SMM_BASE2_PROTOCOL;
typedef struct EFI_SMM_SYSTEM_TABLE2 EFI_SMM_SYSTEM_TABLE2;
typedef EFI_STATUS(EFIAPI *EFI_SMM_GET_SMST_LOCATION2)(
    const EFI_SMM_BASE2_PROTOCOL *This, EFI_SMM_SYSTEM_TABLE2 **Smst);
typedef EFI_STATUS(EFIAPI *EFI_SMM_INSIDE_OUT2)(
    const EFI_SMM_BASE2_PROTOCOL *This, BOOLEAN *InSmram);
typedef EFI_STATUS(EFIAPI *EFI_SMM_HANDLER_ENTRY_POINT2)(
    EFI_HANDLE DispatchHandle, const VOID *Context, VOID *CommBuffer,
    UINTN *CommBufferSize);
typedef EFI_STATUS(EFIAPI *EFI_SMM_INTERRUPT_REGISTER)(
    EFI_SMM_HANDLER_ENTRY_POINT2 Handler, const EFI_GUID *HandlerType,
    EFI_HANDLE *DispatchHandle);
typedef EFI_STATUS(EFIAPI *EFI_SMM_INTERRUPT_UNREGISTER)(
    EFI_HANDLE DispatchHandle);
typedef EFI_STATUS(EFIAPI *EFI_ALLOCATE_POOL)(UINT32 PoolType, UINTN Size,
                                              VOID **Buffer);
typedef EFI_STATUS(EFIAPI *EFI_FREE_POOL_SMM)(VOID *Buffer);
typedef VOID(EFIAPI *SERIAL_PRINT)(const char *Text);
typedef VOID(EFIAPI *SERIAL_HEX64)(UINT64 Value);
typedef EFI_STATUS(EFIAPI *REGISTER_SMI_HANDLER)(
    EFI_SMM_HANDLER_ENTRY_POINT2 Handler, const EFI_GUID *HandlerType,
    EFI_HANDLE *DispatchHandle);
typedef EFI_STATUS(EFIAPI *UNREGISTER_SMI_HANDLER)(
    EFI_HANDLE DispatchHandle);
typedef EFI_STATUS(EFIAPI *ALLOCATE_POOL)(UINTN Size, VOID **Buffer);
typedef EFI_STATUS(EFIAPI *FREE_POOL)(VOID *Buffer);

typedef struct EFI_SMM_SW_DISPATCH2_PROTOCOL EFI_SMM_SW_DISPATCH2_PROTOCOL;

typedef struct {
  UINTN SwSmiInputValue;
} EFI_SMM_SW_REGISTER_CONTEXT;

typedef EFI_STATUS(EFIAPI *EFI_SMM_SW_REGISTER2)(
    const EFI_SMM_SW_DISPATCH2_PROTOCOL *This,
    EFI_SMM_HANDLER_ENTRY_POINT2 DispatchFunction,
    EFI_SMM_SW_REGISTER_CONTEXT *RegisterContext, EFI_HANDLE *DispatchHandle);
typedef EFI_STATUS(EFIAPI *EFI_SMM_SW_UNREGISTER2)(
    const EFI_SMM_SW_DISPATCH2_PROTOCOL *This, EFI_HANDLE DispatchHandle);

struct EFI_SMM_SW_DISPATCH2_PROTOCOL {
  EFI_SMM_SW_REGISTER2 Register;
  EFI_SMM_SW_UNREGISTER2 UnRegister;
  UINTN MaximumSwiValue;
};

struct EFI_SMM_BASE2_PROTOCOL {
  EFI_SMM_INSIDE_OUT2 InSmm;
  EFI_SMM_GET_SMST_LOCATION2 GetSmstLocation;
};

typedef struct {
  VOID *Read;
  VOID *Write;
} EFI_SMM_IO_ACCESS2;

typedef struct {
  EFI_SMM_IO_ACCESS2 Mem;
  EFI_SMM_IO_ACCESS2 Io;
} EFI_SMM_CPU_IO2_PROTOCOL;

struct EFI_SMM_SYSTEM_TABLE2 {
  EFI_TABLE_HEADER Hdr;
  CHAR16 *SmmFirmwareVendor;
  UINT32 SmmFirmwareRevision;
  VOID *SmmInstallConfigurationTable;
  EFI_SMM_CPU_IO2_PROTOCOL SmmIo;
  EFI_ALLOCATE_POOL SmmAllocatePool;
  EFI_FREE_POOL_SMM SmmFreePool;
  VOID *SmmAllocatePages;
  VOID *SmmFreePages;
  VOID *SmmStartupThisAp;
  UINTN CurrentlyExecutingCpu;
  UINTN NumberOfCpus;
  UINTN *CpuSaveStateSize;
  VOID **CpuSaveState;
  UINTN NumberOfTableEntries;
  VOID *SmmConfigurationTable;
  VOID *SmmInstallProtocolInterface;
  VOID *SmmUninstallProtocolInterface;
  VOID *SmmHandleProtocol;
  VOID *SmmRegisterProtocolNotify;
  VOID *SmmLocateHandle;
  VOID *SmmLocateProtocol;
  VOID *SmiManage;
  EFI_SMM_INTERRUPT_REGISTER SmiHandlerRegister;
  EFI_SMM_INTERRUPT_UNREGISTER SmiHandlerUnRegister;
};

typedef struct {
  UINT32 Reason;
  UINT32 Generation;
  UINT32 PayloadSize;
  EFI_SYSTEM_TABLE *SystemTable;
  EFI_SMM_SYSTEM_TABLE2 *Smst;
  SERIAL_PRINT SerialPrint;
  SERIAL_HEX64 SerialHex64;
  REGISTER_SMI_HANDLER RegisterSmiHandler;
  UNREGISTER_SMI_HANDLER UnregisterSmiHandler;
  ALLOCATE_POOL AllocatePool;
  FREE_POOL FreePool;
  VOID *PayloadBase;
  EFI_SMM_INTERRUPT_REGISTER RawSmiHandlerRegister;
  EFI_SMM_INTERRUPT_UNREGISTER RawSmiHandlerUnRegister;
  VOID *SmmLocateProtocol;
} PAYLOAD_CONTEXT;

typedef struct {
  UINT64 Magic;
  UINT32 HeaderSize;
  UINT32 TotalSize;
  UINT32 PayloadCapacity;
  UINT32 PayloadSize;
  UINT64 PayloadHash;
  UINT64 PayloadOffset;
  UINT32 DebugLogSize;
  UINT32 DebugLogCapacity;
  UINT8 DebugLog[MAILBOX_LOG_CAPACITY];
} MAILBOX;

typedef struct {
  UINT64 Magic;
  UINT32 Command;
  UINT32 DataSize;
  UINT64 Offset;
  UINT64 PayloadSize;
  UINT64 PayloadHash;
  UINT64 Sequence;
  UINT8 Data[1];
} WMI_REQUEST;

typedef struct {
  UINT64 Magic;
  UINT32 Size;
  UINT32 Status;
  UINT32 Loaded;
  UINT32 Generation;
  UINT32 LastCommand;
  UINT32 DebugLogSize;
  UINT64 Result;
  UINT64 Sequence;
  UINT8 DebugLog[464];
} WMI_RESPONSE;

#define WMI_REQUEST_HEADER_SIZE OFFSET_OF(WMI_REQUEST, Data)
#define WMI_RESPONSE_LOG_CAPACITY \
  (WMI_RESPONSE_SIZE - (sizeof(WMI_RESPONSE) - 464U))

struct EFI_BOOT_SERVICES {
  EFI_TABLE_HEADER Hdr;
  VOID *RaiseTPL;
  VOID *RestoreTPL;
  VOID *AllocatePages;
  VOID *FreePages;
  VOID *GetMemoryMap;
  VOID *AllocatePool;
  EFI_FREE_POOL FreePool;
  EFI_CREATE_EVENT CreateEvent;
  VOID *SetTimer;
  VOID *WaitForEvent;
  VOID *SignalEvent;
  EFI_CLOSE_EVENT CloseEvent;
  VOID *CheckEvent;
  VOID *InstallProtocolInterface;
  VOID *ReinstallProtocolInterface;
  VOID *UninstallProtocolInterface;
  EFI_HANDLE_PROTOCOL HandleProtocol;
  VOID *Reserved;
  EFI_REGISTER_PROTOCOL_NOTIFY RegisterProtocolNotify;
  VOID *LocateHandle;
  VOID *LocateDevicePath;
  VOID *InstallConfigurationTable;
  VOID *LoadImage;
  VOID *StartImage;
  VOID *Exit;
  VOID *UnloadImage;
  VOID *ExitBootServices;
  VOID *GetNextMonotonicCount;
  VOID *Stall;
  VOID *SetWatchdogTimer;
  VOID *ConnectController;
  VOID *DisconnectController;
  VOID *OpenProtocol;
  VOID *CloseProtocol;
  VOID *OpenProtocolInformation;
  VOID *ProtocolsPerHandle;
  EFI_LOCATE_HANDLE_BUFFER LocateHandleBuffer;
  EFI_LOCATE_PROTOCOL LocateProtocol;
  VOID *InstallMultipleProtocolInterfaces;
  VOID *UninstallMultipleProtocolInterfaces;
  VOID *CalculateCrc32;
  VOID *CopyMem;
  VOID *SetMem;
  VOID *CreateEventEx;
};

struct EFI_SYSTEM_TABLE {
  EFI_TABLE_HEADER Hdr;
  CHAR16 *FirmwareVendor;
  UINT32 FirmwareRevision;
  EFI_HANDLE ConsoleInHandle;
  VOID *ConIn;
  EFI_HANDLE ConsoleOutHandle;
  VOID *ConOut;
  EFI_HANDLE StandardErrorHandle;
  VOID *StdErr;
  VOID *RuntimeServices;
  EFI_BOOT_SERVICES *BootServices;
  UINTN NumberOfTableEntries;
  VOID *ConfigurationTable;
};

typedef EFI_STATUS(EFIAPI *EFI_FILE_OPEN)(EFI_FILE_PROTOCOL *This,
                                          EFI_FILE_PROTOCOL **NewHandle,
                                          CHAR16 *FileName, UINT64 OpenMode,
                                          UINT64 Attributes);
typedef EFI_STATUS(EFIAPI *EFI_FILE_CLOSE)(EFI_FILE_PROTOCOL *This);
typedef EFI_STATUS(EFIAPI *EFI_FILE_READ)(EFI_FILE_PROTOCOL *This,
                                          UINTN *BufferSize, VOID *Buffer);

struct EFI_FILE_PROTOCOL {
  UINT64 Revision;
  EFI_FILE_OPEN Open;
  EFI_FILE_CLOSE Close;
  VOID *Delete;
  EFI_FILE_READ Read;
  VOID *Write;
  VOID *GetPosition;
  VOID *SetPosition;
  VOID *GetInfo;
  VOID *SetInfo;
  VOID *Flush;
};

typedef EFI_STATUS(EFIAPI *EFI_OPEN_VOLUME)(
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *This, EFI_FILE_PROTOCOL **Root);

struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL {
  UINT64 Revision;
  EFI_OPEN_VOLUME OpenVolume;
};

typedef EFI_STATUS(EFIAPI *PAYLOAD_ENTRY)(
    PAYLOAD_CONTEXT *Context);

static EFI_GUID gEfiSimpleFileSystemProtocolGuid = {
    0x0964e5b22,
    0x6459,
    0x11d2,
    {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}};
static EFI_GUID gEfiSmmBase2ProtocolGuid = {
    0xf4ccbfb7,
    0xf6e0,
    0x47fd,
    {0x9d, 0xd4, 0x10, 0xa8, 0xf1, 0x50, 0xc1, 0x91}};
static EFI_GUID gCommunicationGuid = {
    0x2b29c9ad,
    0x2d8f,
    0x4f5e,
    {0x97, 0x4e, 0x5d, 0xe5, 0x54, 0x2e, 0x40, 0x31}};
static EFI_GUID gEfiSmmSwDispatch2ProtocolGuid = {
    0x18a3c6dc,
    0x5eea,
    0x48c8,
    {0xa1, 0xc1, 0xb5, 0x33, 0x89, 0xf9, 0x89, 0x99}};

static UINT8 gPayloadFile[PAYLOAD_FILE_LIMIT];
static UINT8 gPayloadImage[PAYLOAD_IMAGE_LIMIT];
static EFI_SYSTEM_TABLE *gSystemTable;
static EFI_HANDLE gImageHandle;
static UINTN gPayloadLoaded;
static UINTN gPayloadSize;
static UINT32 gPayloadGeneration;
static PAYLOAD_ENTRY gPayloadEntry;
static EFI_SMM_SYSTEM_TABLE2 *gSmst;
static EFI_HANDLE gCommDispatchHandle;
static EFI_HANDLE gControlSwDispatchHandle;
static MAILBOX *gMailbox;
static UINT64 gMailboxPhysical;
static UINT32 gMailboxSize;
static UINT32 gControlSwSmiValue = SW_SMI_VALUE;
static PAYLOAD_CONTEXT gPayloadContext;
static EFI_HANDLE gTrackedHandlers[MAX_TRACKED_HANDLERS];
static VOID *gTrackedPools[MAX_TRACKED_POOLS];
static UINT64 gWmiSequence;

static VOID IoWait(VOID) { __outbyte(0x80, 0); }

static VOID MailboxLogReset(VOID) {
  if (gMailbox == 0 || gMailbox->Magic != MAILBOX_MAGIC ||
      gMailbox->DebugLogCapacity == 0) {
    return;
  }
  gMailbox->DebugLogSize = 0;
  gMailbox->DebugLog[0] = 0;
}

static VOID MailboxLogChar(char Ch) {
  UINT32 Size;
  UINT32 Capacity;

  if (gMailbox == 0 || gMailbox->Magic != MAILBOX_MAGIC ||
      gMailbox->DebugLogCapacity == 0) {
    return;
  }
  Capacity = gMailbox->DebugLogCapacity;
  if (Capacity == 0 || Capacity > MAILBOX_LOG_CAPACITY) {
    Capacity = MAILBOX_LOG_CAPACITY;
  }
  Size = gMailbox->DebugLogSize;
  if (Size + 1 >= Capacity) {
    return;
  }
  gMailbox->DebugLog[Size] = (UINT8)Ch;
  gMailbox->DebugLogSize = Size + 1;
  gMailbox->DebugLog[Size + 1] = 0;
}

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
  MailboxLogChar(Ch);
}

static VOID EFIAPI SerialPrint(const char *Text) {
  while (*Text) {
    SerialPutChar(*Text++);
  }
}

static VOID EFIAPI SerialHex64(UINT64 Value) {
  char Hex[] = "0123456789ABCDEF";
  for (INTN Shift = 60; Shift >= 0; Shift -= 4) {
    SerialPutChar(Hex[(Value >> Shift) & 0xF]);
  }
}

static VOID ZeroMem(VOID *Buffer, UINTN Size) {
  UINT8 *Ptr = (UINT8 *)Buffer;
  while (Size--) {
    *Ptr++ = 0;
  }
}

static VOID CopyMemLocal(VOID *Destination, const VOID *Source, UINTN Size) {
  UINT8 *Dst = (UINT8 *)Destination;
  const UINT8 *Src = (const UINT8 *)Source;
  while (Size--) {
    *Dst++ = *Src++;
  }
}

static UINT64 Hash64(const VOID *Buffer, UINTN Size) {
  const UINT8 *Data = (const UINT8 *)Buffer;
  UINT64 Hash = 0xCBF29CE484222325ULL;
  while (Size--) {
    Hash ^= *Data++;
    Hash *= 0x100000001B3ULL;
  }
  return Hash;
}

static EFI_STATUS EFIAPI HostRegisterSmiHandler(
    EFI_SMM_HANDLER_ENTRY_POINT2 Handler, const EFI_GUID *HandlerType,
    EFI_HANDLE *DispatchHandle) {
  EFI_STATUS Status;
  if (gSmst->SmiHandlerRegister == 0 || DispatchHandle == 0) {
    return EFI_INVALID_PARAMETER;
  }
  Status = gSmst->SmiHandlerRegister(Handler, HandlerType, DispatchHandle);
  if (!EFI_ERROR(Status)) {
    for (UINTN Index = 0; Index < MAX_TRACKED_HANDLERS; Index++) {
      if (gTrackedHandlers[Index] == 0) {
        gTrackedHandlers[Index] = *DispatchHandle;
        return Status;
      }
    }
    SerialPrint("handler tracking full\n");
  }
  return Status;
}

static EFI_STATUS EFIAPI HostUnregisterSmiHandler(EFI_HANDLE DispatchHandle) {
  EFI_STATUS Status;
  if (gSmst->SmiHandlerUnRegister == 0 || DispatchHandle == 0) {
    return EFI_INVALID_PARAMETER;
  }
  Status = gSmst->SmiHandlerUnRegister(DispatchHandle);
  if (!EFI_ERROR(Status)) {
    for (UINTN Index = 0; Index < MAX_TRACKED_HANDLERS; Index++) {
      if (gTrackedHandlers[Index] == DispatchHandle) {
        gTrackedHandlers[Index] = 0;
      }
    }
  }
  return Status;
}

static EFI_STATUS EFIAPI HostAllocatePool(UINTN Size, VOID **Buffer) {
  EFI_STATUS Status;
  if (gSmst->SmmAllocatePool == 0 || Buffer == 0) {
    return EFI_INVALID_PARAMETER;
  }
  Status = gSmst->SmmAllocatePool(EfiRuntimeServicesData, Size, Buffer);
  if (!EFI_ERROR(Status) && *Buffer != 0) {
    for (UINTN Index = 0; Index < MAX_TRACKED_POOLS; Index++) {
      if (gTrackedPools[Index] == 0) {
        gTrackedPools[Index] = *Buffer;
        return Status;
      }
    }
    SerialPrint("pool tracking full\n");
  }
  return Status;
}

static EFI_STATUS EFIAPI HostFreePool(VOID *Buffer) {
  EFI_STATUS Status;
  if (gSmst->SmmFreePool == 0 || Buffer == 0) {
    return EFI_INVALID_PARAMETER;
  }
  Status = gSmst->SmmFreePool(Buffer);
  if (!EFI_ERROR(Status)) {
    for (UINTN Index = 0; Index < MAX_TRACKED_POOLS; Index++) {
      if (gTrackedPools[Index] == Buffer) {
        gTrackedPools[Index] = 0;
      }
    }
  }
  return Status;
}

static VOID PreparePayloadContext(UINT32 Reason) {
  ZeroMem(&gPayloadContext, sizeof(gPayloadContext));
  gPayloadContext.Reason = Reason;
  gPayloadContext.Generation = gPayloadGeneration;
  gPayloadContext.PayloadSize = (UINT32)gPayloadSize;
  gPayloadContext.SystemTable = gSystemTable;
  gPayloadContext.Smst = gSmst;
  gPayloadContext.SerialPrint = SerialPrint;
  gPayloadContext.SerialHex64 = SerialHex64;
  gPayloadContext.RegisterSmiHandler = HostRegisterSmiHandler;
  gPayloadContext.UnregisterSmiHandler = HostUnregisterSmiHandler;
  gPayloadContext.AllocatePool = HostAllocatePool;
  gPayloadContext.FreePool = HostFreePool;
  gPayloadContext.PayloadBase = gPayloadImage;
  gPayloadContext.RawSmiHandlerRegister = gSmst->SmiHandlerRegister;
  gPayloadContext.RawSmiHandlerUnRegister = gSmst->SmiHandlerUnRegister;
  gPayloadContext.SmmLocateProtocol = gSmst->SmmLocateProtocol;
}

VOID *memset(VOID *Destination, int Value, size_t Size) {
  UINT8 *Dst = (UINT8 *)Destination;
  while (Size--) {
    *Dst++ = (UINT8)Value;
  }
  return Destination;
}

VOID *memcpy(VOID *Destination, const VOID *Source, size_t Size) {
  UINT8 *Dst = (UINT8 *)Destination;
  const UINT8 *Src = (const UINT8 *)Source;
  while (Size--) {
    *Dst++ = *Src++;
  }
  return Destination;
}

static UINT16 Read16(const VOID *Ptr) {
  const UINT8 *B = (const UINT8 *)Ptr;
  return (UINT16)(B[0] | (B[1] << 8));
}

static UINT32 Read32(const VOID *Ptr) {
  const UINT8 *B = (const UINT8 *)Ptr;
  return (UINT32)B[0] | ((UINT32)B[1] << 8) | ((UINT32)B[2] << 16) |
         ((UINT32)B[3] << 24);
}

static UINT64 Read64(const VOID *Ptr) {
  const UINT8 *B = (const UINT8 *)Ptr;
  return (UINT64)Read32(B) | ((UINT64)Read32(B + 4) << 32);
}

static VOID Write32(VOID *Ptr, UINT32 Value) {
  UINT8 *B = (UINT8 *)Ptr;
  B[0] = (UINT8)Value;
  B[1] = (UINT8)(Value >> 8);
  B[2] = (UINT8)(Value >> 16);
  B[3] = (UINT8)(Value >> 24);
}

static VOID Write64(VOID *Ptr, UINT64 Value) {
  Write32(Ptr, (UINT32)Value);
  Write32((UINT8 *)Ptr + 4, (UINT32)(Value >> 32));
}

static EFI_STATUS ReadPayloadFile(EFI_SYSTEM_TABLE *SystemTable,
                                  UINTN *PayloadSize) {
  static CHAR16 PayloadPath[] = {
      '\\', 'E', 'F', 'I', '\\', 'S', 'M', 'M', '\\', 'P', 'A',
      'Y',  'L', 'O', 'A', 'D',  '.', 'E', 'F', 'I', 0};
  EFI_BOOT_SERVICES *Bs = SystemTable->BootServices;
  EFI_HANDLE *Handles = 0;
  UINTN HandleCount = 0;
  EFI_STATUS Status;

  *PayloadSize = 0;
  Status = Bs->LocateHandleBuffer(2, &gEfiSimpleFileSystemProtocolGuid, 0,
                                  &HandleCount, &Handles);
  if (EFI_ERROR(Status)) {
    SerialPrint("LocateHandleBuffer(SimpleFS) failed ");
    SerialHex64(Status);
    SerialPrint("\n");
    return Status;
  }

  for (UINTN Index = 0; Index < HandleCount; Index++) {
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Fs = 0;
    EFI_FILE_PROTOCOL *Root = 0;
    EFI_FILE_PROTOCOL *File = 0;
    UINTN ReadSize = PAYLOAD_FILE_LIMIT;

    Status = Bs->HandleProtocol(Handles[Index],
                                &gEfiSimpleFileSystemProtocolGuid,
                                (VOID **)&Fs);
    if (EFI_ERROR(Status) || Fs == 0) {
      continue;
    }

    Status = Fs->OpenVolume(Fs, &Root);
    if (EFI_ERROR(Status) || Root == 0) {
      continue;
    }

    Status = Root->Open(Root, &File, PayloadPath, 0x0000000000000001ULL, 0);
    if (!EFI_ERROR(Status) && File != 0) {
      ZeroMem(gPayloadFile, PAYLOAD_FILE_LIMIT);
      Status = File->Read(File, &ReadSize, gPayloadFile);
      File->Close(File);
      Root->Close(Root);
      if (!EFI_ERROR(Status) && ReadSize > 0 && ReadSize < PAYLOAD_FILE_LIMIT) {
        *PayloadSize = ReadSize;
        Bs->FreePool(Handles);
        SerialPrint("loaded PAYLOAD.EFI bytes=0x");
        SerialHex64(ReadSize);
        SerialPrint("\n");
        return EFI_SUCCESS;
      }
      SerialPrint("read failed or file too large\n");
    } else {
      Root->Close(Root);
    }
  }

  if (Handles != 0) {
    Bs->FreePool(Handles);
  }
  SerialPrint("\\EFI\\SMM\\PAYLOAD.EFI not found\n");
  return EFI_NOT_FOUND;
}

static UINT8 *RvaToPtr(UINT8 *Image, UINT32 SizeOfImage, UINT32 Rva,
                       UINT32 Length) {
  if (Rva > SizeOfImage || Length > SizeOfImage - Rva) {
    return 0;
  }
  return Image + Rva;
}

static EFI_STATUS LoadPe32Plus(UINT8 *File, UINTN FileSize,
                               PAYLOAD_ENTRY *EntryPoint) {
  UINT32 PeOff;
  UINT16 NumberOfSections;
  UINT16 SizeOfOptionalHeader;
  UINT8 *Nt;
  UINT8 *Optional;
  UINT8 *Section;
  UINT64 PreferredBase;
  UINT32 SizeOfImage;
  UINT32 SizeOfHeaders;
  UINT32 AddressOfEntryPoint;
  UINT32 ImportRva;
  UINT32 ImportSize;
  UINT32 RelocRva;
  UINT32 RelocSize;
  UINT64 Delta;

  *EntryPoint = 0;
  if (FileSize < 0x100 || Read16(File) != 0x5A4D) {
    return EFI_LOAD_ERROR;
  }
  PeOff = Read32(File + 0x3C);
  if (PeOff > FileSize || FileSize - PeOff < 0x108) {
    return EFI_LOAD_ERROR;
  }
  Nt = File + PeOff;
  if (Read32(Nt) != 0x00004550) {
    return EFI_LOAD_ERROR;
  }
  if (Read16(Nt + 4) != 0x8664) {
    SerialPrint("payload is not AMD64\n");
    return EFI_UNSUPPORTED;
  }

  NumberOfSections = Read16(Nt + 6);
  SizeOfOptionalHeader = Read16(Nt + 20);
  Optional = Nt + 24;
  if (Read16(Optional) != 0x20B || SizeOfOptionalHeader < 0xF0) {
    SerialPrint("payload is not PE32+\n");
    return EFI_UNSUPPORTED;
  }

  AddressOfEntryPoint = Read32(Optional + 0x10);
  PreferredBase = Read64(Optional + 0x18);
  SizeOfImage = Read32(Optional + 0x38);
  SizeOfHeaders = Read32(Optional + 0x3C);
  ImportRva = Read32(Optional + 0x70 + 1 * 8);
  ImportSize = Read32(Optional + 0x70 + 1 * 8 + 4);
  RelocRva = Read32(Optional + 0x70 + 5 * 8);
  RelocSize = Read32(Optional + 0x70 + 5 * 8 + 4);

  if (ImportRva != 0 || ImportSize != 0) {
    SerialPrint("payload imports are unsupported\n");
    return EFI_UNSUPPORTED;
  }
  if (SizeOfImage == 0 || SizeOfImage > PAYLOAD_IMAGE_LIMIT ||
      SizeOfHeaders > FileSize) {
    SerialPrint("payload image size rejected\n");
    return EFI_UNSUPPORTED;
  }

  ZeroMem(gPayloadImage, PAYLOAD_IMAGE_LIMIT);
  CopyMemLocal(gPayloadImage, File, SizeOfHeaders);

  Section = Optional + SizeOfOptionalHeader;
  for (UINTN Index = 0; Index < NumberOfSections; Index++) {
    UINT8 *Sh = Section + Index * 40;
    UINT32 VirtualSize = Read32(Sh + 8);
    UINT32 VirtualAddress = Read32(Sh + 12);
    UINT32 SizeOfRawData = Read32(Sh + 16);
    UINT32 PointerToRawData = Read32(Sh + 20);
    UINT32 CopySize = SizeOfRawData;
    if (VirtualSize != 0 && VirtualSize < CopySize) {
      CopySize = VirtualSize;
    }
    if (CopySize == 0) {
      continue;
    }
    if (PointerToRawData > FileSize || CopySize > FileSize - PointerToRawData ||
        RvaToPtr(gPayloadImage, SizeOfImage, VirtualAddress, CopySize) == 0) {
      SerialPrint("payload section bounds rejected\n");
      return EFI_LOAD_ERROR;
    }
    CopyMemLocal(gPayloadImage + VirtualAddress, File + PointerToRawData,
                 CopySize);
  }

  Delta = (UINT64)(UINTN)gPayloadImage - PreferredBase;
  if (Delta != 0) {
    UINT32 Offset = 0;
    if (RelocRva == 0 || RelocSize == 0) {
      SerialPrint("payload has no relocation directory; assuming RIP-relative image\n");
      *EntryPoint =
          (PAYLOAD_ENTRY)(VOID *)(gPayloadImage + AddressOfEntryPoint);
      return EFI_SUCCESS;
    }
    if (RvaToPtr(gPayloadImage, SizeOfImage, RelocRva, RelocSize) == 0) {
      SerialPrint("relocation directory bounds rejected\n");
      return EFI_LOAD_ERROR;
    }
    while (Offset + 8 <= RelocSize) {
      UINT8 *Block = gPayloadImage + RelocRva + Offset;
      UINT32 PageRva = Read32(Block);
      UINT32 BlockSize = Read32(Block + 4);
      UINT32 Count;
      if (BlockSize < 8 || Offset + BlockSize > RelocSize) {
        return EFI_LOAD_ERROR;
      }
      Count = (BlockSize - 8) / 2;
      for (UINT32 I = 0; I < Count; I++) {
        UINT16 Item = Read16(Block + 8 + I * 2);
        UINT16 Type = Item >> 12;
        UINT16 FixupOffset = Item & 0x0FFF;
        UINT8 *Fixup = RvaToPtr(gPayloadImage, SizeOfImage,
                                PageRva + FixupOffset, 8);
        if (Type == 0) {
          continue;
        } else if (Type == 10 && Fixup != 0) {
          Write64(Fixup, Read64(Fixup) + Delta);
        } else if (Type == 3 && Fixup != 0) {
          Write32(Fixup, Read32(Fixup) + (UINT32)Delta);
        } else {
          SerialPrint("unsupported relocation type\n");
          return EFI_UNSUPPORTED;
        }
      }
      Offset += BlockSize;
    }
  }

  *EntryPoint =
      (PAYLOAD_ENTRY)(VOID *)(gPayloadImage + AddressOfEntryPoint);
  return EFI_SUCCESS;
}

static VOID CleanupTrackedResources(VOID) {
  if (gSmst->SmiHandlerUnRegister != 0) {
    for (UINTN Index = 0; Index < MAX_TRACKED_HANDLERS; Index++) {
      if (gTrackedHandlers[Index] != 0) {
        gSmst->SmiHandlerUnRegister(gTrackedHandlers[Index]);
        gTrackedHandlers[Index] = 0;
      }
    }
  }
  if (gSmst->SmmFreePool != 0) {
    for (UINTN Index = 0; Index < MAX_TRACKED_POOLS; Index++) {
      if (gTrackedPools[Index] != 0) {
        gSmst->SmmFreePool(gTrackedPools[Index]);
        gTrackedPools[Index] = 0;
      }
    }
  }
}

static EFI_STATUS UnloadPayload(const char *Reason) {
  EFI_STATUS Status = EFI_SUCCESS;

  if (gPayloadLoaded == 0 || gPayloadEntry == 0) {
    CleanupTrackedResources();
    gPayloadLoaded = 0;
    gPayloadEntry = 0;
    gPayloadSize = 0;
    return EFI_SUCCESS;
  }

  SerialPrint("unloading payload: ");
  SerialPrint(Reason);
  SerialPrint("\n");
  PreparePayloadContext(REASON_UNLOAD);
  Status = gPayloadEntry(&gPayloadContext);
  if (EFI_ERROR(Status)) {
    SerialPrint("unload failed ");
    SerialHex64(Status);
    SerialPrint("\n");
  } else {
    SerialPrint("unload ok\n");
  }

  CleanupTrackedResources();
  gPayloadLoaded = 0;
  gPayloadEntry = 0;
  gPayloadSize = 0;
  ZeroMem(gPayloadImage, PAYLOAD_IMAGE_LIMIT);
  gPayloadGeneration++;
  return Status;
}

static EFI_STATUS InstallPayloadBytes(const UINT8 *Payload, UINTN PayloadSize,
                                      const char *Reason) {
  EFI_STATUS Status;
  PAYLOAD_ENTRY PayloadEntry = 0;

  if (Payload == 0 || PayloadSize == 0 || PayloadSize >= PAYLOAD_FILE_LIMIT) {
    SerialPrint("payload size rejected\n");
    return EFI_INVALID_PARAMETER;
  }

  SerialPrint("installing payload: ");
  SerialPrint(Reason);
  SerialPrint(" bytes=0x");
  SerialHex64(PayloadSize);
  SerialPrint("\n");

  if (Payload != gPayloadFile) {
    ZeroMem(gPayloadFile, PAYLOAD_FILE_LIMIT);
    CopyMemLocal(gPayloadFile, Payload, PayloadSize);
  }
  Status = LoadPe32Plus(gPayloadFile, PayloadSize, &PayloadEntry);
  if (EFI_ERROR(Status) || PayloadEntry == 0) {
    SerialPrint("payload mapper failed ");
    SerialHex64(Status);
    SerialPrint("\n");
    return Status;
  }

  gPayloadLoaded = 1;
  gPayloadEntry = PayloadEntry;
  gPayloadSize = PayloadSize;
  PreparePayloadContext(REASON_LOAD);
  SerialPrint("calling payload entry ABI\n");
  Status = PayloadEntry(&gPayloadContext);
  if (EFI_ERROR(Status)) {
    SerialPrint("load failed ");
    SerialHex64(Status);
    SerialPrint("\n");
    CleanupTrackedResources();
    gPayloadLoaded = 0;
    gPayloadEntry = 0;
    gPayloadSize = 0;
    ZeroMem(gPayloadImage, PAYLOAD_IMAGE_LIMIT);
  } else {
    SerialPrint("load ok bytes=0x");
    SerialHex64(PayloadSize);
    SerialPrint(" gen=0x");
    SerialHex64(gPayloadGeneration);
    SerialPrint("\n");
  }
  return Status;
}

static EFI_STATUS TryLoadPayload(const char *Reason) {
  EFI_STATUS Status;
  UINTN PayloadSize = 0;

  if (gPayloadLoaded != 0) {
    return EFI_SUCCESS;
  }

  SerialPrint("payload attempt: ");
  SerialPrint(Reason);
  SerialPrint("\n");

  Status = ReadPayloadFile(gSystemTable, &PayloadSize);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  return InstallPayloadBytes(gPayloadFile, PayloadSize, Reason);
}

static EFI_STATUS AcceptPayloadBytes(const UINT8 *Payload, UINTN PayloadSize,
                                     const char *Reason) {
  if (gPayloadLoaded != 0) {
    return EFI_SUCCESS;
  }
  return InstallPayloadBytes(Payload, PayloadSize, Reason);
}

static VOID ConfigureMailbox(UINT64 MailboxPhysical, UINT32 MailboxSize,
                             UINT32 SwSmiValue) {
  if (MailboxPhysical == 0 || MailboxSize < MAILBOX_TOTAL_SIZE) {
    return;
  }
  gMailboxPhysical = MailboxPhysical;
  gMailboxSize = MailboxSize;
  gMailbox = (MAILBOX *)(UINTN)MailboxPhysical;
  if (SwSmiValue != 0) {
    gControlSwSmiValue = SwSmiValue;
  }
  if (gMailbox->Magic != MAILBOX_MAGIC ||
      gMailbox->HeaderSize < sizeof(MAILBOX) ||
      gMailbox->TotalSize < MAILBOX_TOTAL_SIZE ||
      gMailbox->PayloadOffset < gMailbox->HeaderSize ||
      gMailbox->PayloadOffset + gMailbox->PayloadCapacity >
          gMailbox->TotalSize ||
      gMailbox->DebugLogCapacity == 0 ||
      gMailbox->DebugLogCapacity > MAILBOX_LOG_CAPACITY) {
    ZeroMem(gMailbox, sizeof(MAILBOX));
    gMailbox->Magic = MAILBOX_MAGIC;
    gMailbox->HeaderSize = MAILBOX_HEADER_SIZE;
    gMailbox->TotalSize = MailboxSize;
    gMailbox->PayloadCapacity = MAILBOX_PAYLOAD_CAPACITY;
    gMailbox->PayloadOffset = MAILBOX_HEADER_SIZE;
    gMailbox->DebugLogCapacity = MAILBOX_LOG_CAPACITY;
    gMailbox->DebugLogSize = 0;
  }
  SerialPrint("mailbox configured base=0x");
  SerialHex64(MailboxPhysical);
  SerialPrint(" size=0x");
  SerialHex64(MailboxSize);
  SerialPrint(" sw=0x");
  SerialHex64(gControlSwSmiValue);
  SerialPrint("\n");
}

static EFI_STATUS ReloadPayloadFromMailbox(VOID) {
  const UINT8 *Payload;
  UINTN PayloadSize;
  UINT64 ExpectedHash;
  UINT64 ActualHash;
  EFI_STATUS Status;

  if (gMailbox == 0) {
    return EFI_NOT_FOUND;
  }
  PayloadSize = gMailbox->PayloadSize;
  if (PayloadSize == 0 || PayloadSize >= PAYLOAD_FILE_LIMIT ||
      PayloadSize > gMailbox->PayloadCapacity ||
      gMailbox->PayloadOffset < gMailbox->HeaderSize ||
      gMailbox->PayloadOffset + PayloadSize > gMailbox->TotalSize) {
    SerialPrint("hot reload payload bounds rejected\n");
    return EFI_INVALID_PARAMETER;
  }
  Payload = (const UINT8 *)gMailbox + gMailbox->PayloadOffset;
  ExpectedHash = gMailbox->PayloadHash;
  ActualHash = Hash64(Payload, PayloadSize);
  if (ExpectedHash != 0 && ExpectedHash != ActualHash) {
    SerialPrint("hot reload hash mismatch\n");
    return EFI_INVALID_PARAMETER;
  }

  Status = UnloadPayload("hot reload");
  if (EFI_ERROR(Status)) {
    return Status;
  }
  return InstallPayloadBytes(Payload, PayloadSize, "hot reload");
}

static EFI_STATUS DispatchDoorbellToPayload(VOID) {
  EFI_STATUS Status;
  if (gPayloadLoaded == 0 || gPayloadEntry == 0) {
    return EFI_SUCCESS;
  }
  MailboxLogReset();
  PreparePayloadContext(REASON_DOORBELL);
  Status = gPayloadEntry(&gPayloadContext);
  if (EFI_ERROR(Status)) {
    SerialPrint("doorbell failed ");
    SerialHex64(Status);
    SerialPrint("\n");
  }
  return Status;
}

static VOID WriteWmiResponse(UINT32 Command, EFI_STATUS Status) {
  WMI_RESPONSE *Response;
  UINT32 LogSize;

  if (gMailbox == 0 || gMailbox->Magic != MAILBOX_MAGIC) {
    return;
  }
  Response = (WMI_RESPONSE *)((UINT8 *)gMailbox + WMI_RESPONSE_OFFSET);
  ZeroMem(Response, WMI_RESPONSE_SIZE);
  Response->Magic = WMI_RESPONSE_MAGIC;
  Response->Size = WMI_RESPONSE_SIZE;
  Response->Status = (UINT32)Status;
  Response->Loaded = (UINT32)gPayloadLoaded;
  Response->Generation = gPayloadGeneration;
  Response->LastCommand = Command;
  Response->Result = Status;
  Response->Sequence = ++gWmiSequence;
  if (gMailbox->DebugLogCapacity != 0 &&
      gMailbox->DebugLogSize <= gMailbox->DebugLogCapacity) {
    LogSize = gMailbox->DebugLogSize;
    if (LogSize > WMI_RESPONSE_LOG_CAPACITY) {
      LogSize = WMI_RESPONSE_LOG_CAPACITY;
    }
    Response->DebugLogSize = LogSize;
    if (LogSize != 0) {
      CopyMemLocal(Response->DebugLog, gMailbox->DebugLog, LogSize);
    }
  }
}

static EFI_STATUS ProcessWmiRequest(VOID) {
  WMI_REQUEST *Request;
  UINT32 Command;
  UINTN DataSize;
  UINTN PayloadSize;
  UINT64 ActualHash;
  EFI_STATUS Status = EFI_SUCCESS;

  if (gMailbox == 0 || gMailbox->Magic != MAILBOX_MAGIC) {
    return EFI_NOT_FOUND;
  }
  Request = (WMI_REQUEST *)((UINT8 *)gMailbox + MAILBOX_HEADER_SIZE);
  if (Request->Magic != WMI_REQUEST_MAGIC) {
    return DispatchDoorbellToPayload();
  }

  Command = Request->Command;
  DataSize = (UINTN)Request->DataSize;
  MailboxLogReset();

  if (Command == WMI_COMMAND_DOORBELL) {
    Status = DispatchDoorbellToPayload();
  } else if (Command == WMI_COMMAND_PING || Command == WMI_COMMAND_STATUS) {
    Status = EFI_SUCCESS;
  } else if (Command == WMI_COMMAND_UNLOAD) {
    Status = UnloadPayload("WMI unload");
  } else if (Command == WMI_COMMAND_STAGE_CHUNK) {
    if (DataSize > WMI_REQUEST_SIZE - WMI_REQUEST_HEADER_SIZE ||
        Request->Offset + DataSize > PAYLOAD_FILE_LIMIT) {
      Status = EFI_INVALID_PARAMETER;
    } else {
      CopyMemLocal(gPayloadFile + Request->Offset, Request->Data, DataSize);
      Status = EFI_SUCCESS;
    }
  } else if (Command == WMI_COMMAND_RELOAD) {
    PayloadSize = (UINTN)Request->PayloadSize;
    if (PayloadSize == 0 || PayloadSize >= PAYLOAD_FILE_LIMIT) {
      Status = EFI_INVALID_PARAMETER;
    } else {
      ActualHash = Hash64(gPayloadFile, PayloadSize);
      if (Request->PayloadHash != 0 && Request->PayloadHash != ActualHash) {
        SerialPrint("WMI reload hash mismatch\n");
        Status = EFI_INVALID_PARAMETER;
      } else {
        Status = UnloadPayload("WMI reload");
        if (!EFI_ERROR(Status)) {
          Status = InstallPayloadBytes(gPayloadFile, PayloadSize,
                                       "WMI reload");
        }
      }
    }
  } else {
    Status = EFI_UNSUPPORTED;
  }

  Request->Magic = 0;
  WriteWmiResponse(Command, Status);
  return Status;
}

static EFI_STATUS EFIAPI ControlSwSmiHandler(EFI_HANDLE DispatchHandle,
                                                    const VOID *Context,
                                                    VOID *CommBuffer,
                                                   UINTN *CommBufferSize) {
  (void)DispatchHandle;
  (void)Context;
  (void)CommBuffer;
  (void)CommBufferSize;

  SerialInit();
  SerialPrint("control SMI entered\n");

  if (gMailbox == 0 || gMailbox->Magic != MAILBOX_MAGIC ||
      gMailbox->HeaderSize < sizeof(MAILBOX) ||
      gMailbox->TotalSize < MAILBOX_TOTAL_SIZE) {
    return EFI_NOT_FOUND;
  }
  return ProcessWmiRequest();
}

static EFI_STATUS EFIAPI CommunicationHandler(EFI_HANDLE DispatchHandle,
                                                    const VOID *Context,
                                                    VOID *CommBuffer,
                                                    UINTN *CommBufferSize) {
  COMM_MESSAGE *Message = (COMM_MESSAGE *)CommBuffer;
  const UINT8 *Payload;
  UINTN MinimumSize;
  UINTN PayloadSize;
  UINT64 ExpectedHash;
  UINT64 ActualHash;
  (void)DispatchHandle;
  (void)Context;

  if (CommBuffer == 0 || CommBufferSize == 0) {
    return EFI_INVALID_PARAMETER;
  }
  if (*CommBufferSize < COMM_HEADER_SIZE) {
    SerialPrint("comm buffer too small\n");
    return EFI_INVALID_PARAMETER;
  }
  if (Message->Magic != COMM_MAGIC) {
    SerialPrint("comm magic rejected\n");
    return EFI_INVALID_PARAMETER;
  }
  ConfigureMailbox(Message->MailboxPhysical, Message->MailboxSize,
                   Message->SwSmiValue);

  PayloadSize = (UINTN)Message->PayloadSize;
  if (PayloadSize == 0 || PayloadSize >= PAYLOAD_FILE_LIMIT) {
    SerialPrint("comm payload bounds rejected\n");
    return EFI_INVALID_PARAMETER;
  }

  if (Message->Command == COMM_LOAD_INLINE) {
    MinimumSize = COMM_HEADER_SIZE + PayloadSize;
    if (*CommBufferSize < MinimumSize) {
      SerialPrint("comm payload bounds rejected\n");
      return EFI_INVALID_PARAMETER;
    }
    Payload = Message->Payload;
  } else if (Message->Command == COMM_LOAD_MAILBOX) {
    if (gMailbox == 0 ||
        gMailbox->PayloadOffset < gMailbox->HeaderSize ||
        gMailbox->PayloadOffset + PayloadSize > gMailbox->TotalSize ||
        gMailbox->PayloadSize != PayloadSize) {
      SerialPrint("mailbox payload bounds rejected\n");
      return EFI_INVALID_PARAMETER;
    }
    Payload = (const UINT8 *)gMailbox + gMailbox->PayloadOffset;
    ExpectedHash = gMailbox->PayloadHash;
    ActualHash = Hash64(Payload, PayloadSize);
    if (ExpectedHash != 0 && ExpectedHash != ActualHash) {
      SerialPrint("mailbox payload hash mismatch\n");
      return EFI_INVALID_PARAMETER;
    }
  } else {
    SerialPrint("comm command rejected\n");
    return EFI_UNSUPPORTED;
  }

  return AcceptPayloadBytes(Payload, PayloadSize, "SMM communication");
}

static EFI_STATUS RegisterSmmCommunication(VOID) {
  EFI_STATUS Status;
  EFI_SMM_BASE2_PROTOCOL *SmmBase = 0;

  Status = gSystemTable->BootServices->LocateProtocol(&gEfiSmmBase2ProtocolGuid,
                                                      0, (VOID **)&SmmBase);
  if (EFI_ERROR(Status) || SmmBase == 0) {
    SerialPrint("LocateProtocol(SmmBase2) failed ");
    SerialHex64(Status);
    SerialPrint("\n");
    return Status;
  }

  Status = SmmBase->GetSmstLocation(SmmBase, &gSmst);
  if (EFI_ERROR(Status) || gSmst == 0 || gSmst->SmiHandlerRegister == 0) {
    SerialPrint("GetSmstLocation failed ");
    SerialHex64(Status);
    SerialPrint("\n");
    return Status;
  }

  Status = gSmst->SmiHandlerRegister(CommunicationHandler,
                                     &gCommunicationGuid,
                                     &gCommDispatchHandle);
  if (EFI_ERROR(Status)) {
    SerialPrint("SmiHandlerRegister failed ");
    SerialHex64(Status);
    SerialPrint("\n");
    return Status;
  }

  SerialPrint("communication handler registered\n");
  return EFI_SUCCESS;
}

static EFI_STATUS RegisterControlSwSmi(VOID) {
  EFI_STATUS Status;
  EFI_SMM_SW_DISPATCH2_PROTOCOL *SwDispatch = 0;
  EFI_SMM_SW_REGISTER_CONTEXT SwContext;

  if (gSmst->SmmLocateProtocol == 0) {
    return EFI_INVALID_PARAMETER;
  }
  Status = ((EFI_LOCATE_PROTOCOL)gSmst->SmmLocateProtocol)(
      &gEfiSmmSwDispatch2ProtocolGuid, 0, (VOID **)&SwDispatch);
  if (EFI_ERROR(Status) || SwDispatch == 0 || SwDispatch->Register == 0) {
    SerialPrint("SmmSwDispatch2 unavailable ");
    SerialHex64(Status);
    SerialPrint("\n");
    return Status;
  }
  if ((UINTN)gControlSwSmiValue > SwDispatch->MaximumSwiValue) {
    SerialPrint("SW SMI value outside dispatch range\n");
    return EFI_INVALID_PARAMETER;
  }

  SwContext.SwSmiInputValue = gControlSwSmiValue;
  Status = SwDispatch->Register(SwDispatch, ControlSwSmiHandler,
                                &SwContext, &gControlSwDispatchHandle);
  if (EFI_ERROR(Status)) {
    SerialPrint("SW SMI control register failed ");
    SerialHex64(Status);
    SerialPrint("\n");
    return Status;
  }

  SerialPrint("SW SMI control registered value=0x");
  SerialHex64(gControlSwSmiValue);
  SerialPrint("\n");
  return EFI_SUCCESS;
}

EFI_STATUS EFIAPI SmmHostEntry(EFI_HANDLE ImageHandle,
                               EFI_SYSTEM_TABLE *SystemTable) {
  EFI_STATUS Status;

  SerialInit();
  SerialPrint("SMM init\n");

  if (SystemTable == 0 || SystemTable->BootServices == 0) {
    SerialPrint("no system table\n");
    return EFI_INVALID_PARAMETER;
  }
  gSystemTable = SystemTable;
  gImageHandle = ImageHandle;

  Status = RegisterSmmCommunication();
  if (EFI_ERROR(Status)) {
    return EFI_SUCCESS;
  }
  RegisterControlSwSmi();

  Status = TryLoadPayload("initial dispatch");
  if (EFI_ERROR(Status)) {
    SerialPrint("waiting for DXE bridge payload delivery\n");
  }
  return EFI_SUCCESS;
}