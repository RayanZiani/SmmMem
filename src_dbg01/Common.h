#ifndef COMMON_H
#define COMMON_H

typedef unsigned char UINT8;
typedef unsigned char BOOLEAN;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef unsigned long long UINT64;
typedef unsigned long long UINTN;
typedef unsigned long long size_t;
typedef unsigned short CHAR16;
typedef void VOID;
typedef VOID *EFI_HANDLE;
typedef VOID *EFI_EVENT;
typedef UINTN EFI_TPL;
typedef UINT64 EFI_STATUS;
typedef UINT64 EFI_PHYSICAL_ADDRESS;

#define EFIAPI __cdecl

#define EFI_SUCCESS 0
#define EFI_INVALID_PARAMETER 2
#define EFI_UNSUPPORTED 3
#define EFI_OUT_OF_RESOURCES 9
#define EFI_NOT_FOUND 14
#define EFI_ERROR(Status) ((Status) != EFI_SUCCESS)

#define SW_SMI_VALUE 0xD6U
#define MAILBOX_SIZE 0x2000U
#define REQUEST_SIZE 4096U
#define RESPONSE_OFFSET 0x1000U
#define RESPONSE_SIZE 512U
#define RESPONSE_DATA_SIZE 352U
#define NAME_SIZE 64U

#define CONFIG_MAGIC 0x434D4D534D4D5355ULL
#define REQ_MAGIC 0x5145524D4D5355ULL
#define RESP_MAGIC 0x5345524D4D5355ULL

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

#define STATUS_OK 0U
#define EFI_RUNTIME_SERVICES_DATA 6U
#define EFI_VARIABLE_NON_VOLATILE 0x00000001U
#define EFI_VARIABLE_BOOTSERVICE_ACCESS 0x00000002U
#define EFI_VARIABLE_RUNTIME_ACCESS 0x00000004U
#define SERIAL 1U
#define COM1_PORT 0x3F8U
#define DEBUG_MAGIC 0x4742444D454D4D53ULL
#define DEBUG_TRACE_MAGIC 0x45434152544D4D53ULL
#define DEBUG_RECORD_COUNT 48U

#define DBG_DXE_ENTRY 0x100U
#define DBG_DXE_MAILBOX_OK 0x110U
#define DBG_DXE_CONFIG_PUBLISHED 0x120U
#define DBG_DXE_ACPI_LOCATE 0x130U
#define DBG_DXE_SSDT_BUILT 0x140U
#define DBG_DXE_SSDT_INSTALLED 0x150U
#define DBG_DXE_SMM_LOCATE 0x160U
#define DBG_DXE_SMM_COMM_REGION 0x170U
#define DBG_DXE_SMM_CONFIGURED 0x180U
#define DBG_DXE_DONE 0x1FFU

#define DBG_SMM_ENTRY 0x200U
#define DBG_SMM_SMST_OK 0x210U
#define DBG_SMM_COMM_REGISTERED 0x220U
#define DBG_SMM_CONFIG_APPLIED 0x230U
#define DBG_SMM_SW_REGISTERED 0x240U
#define DBG_SMM_DONE 0x2FFU

typedef struct {
  UINT32 Data1;
  UINT16 Data2;
  UINT16 Data3;
  UINT8 Data4[8];
} EFI_GUID;

typedef struct {
  UINT64 Signature;
  UINT32 Revision;
  UINT32 HeaderSize;
  UINT32 CRC32;
  UINT32 Reserved;
} EFI_TABLE_HEADER;

typedef struct {
  EFI_GUID VendorGuid;
  VOID *VendorTable;
} EFI_CONFIGURATION_TABLE;

typedef struct EFI_BOOT_SERVICES EFI_BOOT_SERVICES;
typedef struct EFI_SYSTEM_TABLE EFI_SYSTEM_TABLE;
typedef struct EFI_RUNTIME_SERVICES EFI_RUNTIME_SERVICES;
typedef struct EFI_ACPI_TABLE_PROTOCOL EFI_ACPI_TABLE_PROTOCOL;
typedef struct EFI_SMM_COMMUNICATION_PROTOCOL EFI_SMM_COMMUNICATION_PROTOCOL;
typedef struct EFI_SMM_BASE2_PROTOCOL EFI_SMM_BASE2_PROTOCOL;
typedef struct EFI_SMM_SYSTEM_TABLE2 EFI_SMM_SYSTEM_TABLE2;
typedef struct EFI_SMM_SW_DISPATCH2_PROTOCOL EFI_SMM_SW_DISPATCH2_PROTOCOL;
typedef struct EFI_SMM_VARIABLE_PROTOCOL EFI_SMM_VARIABLE_PROTOCOL;

typedef EFI_STATUS(EFIAPI *EFI_LOCATE_PROTOCOL)(EFI_GUID *Protocol,
                                                VOID *Registration,
                                                VOID **Interface);
typedef enum {
  AllocateAnyPages,
  AllocateMaxAddress,
  AllocateAddress
} EFI_ALLOCATE_TYPE;
typedef EFI_STATUS(EFIAPI *EFI_ALLOCATE_PAGES)(EFI_ALLOCATE_TYPE Type,
                                               UINT32 MemoryType,
                                               UINTN Pages,
                                               EFI_PHYSICAL_ADDRESS *Memory);
typedef EFI_STATUS(EFIAPI *EFI_INSTALL_CONFIGURATION_TABLE)(
    EFI_GUID *Guid, VOID *Table);
typedef VOID(EFIAPI *EFI_EVENT_NOTIFY)(EFI_EVENT Event, VOID *Context);
typedef enum {
  TimerCancel,
  TimerPeriodic,
  TimerRelative
} EFI_TIMER_DELAY;
typedef EFI_STATUS(EFIAPI *EFI_CREATE_EVENT)(UINT32 Type, EFI_TPL NotifyTpl,
                                             EFI_EVENT_NOTIFY NotifyFunction,
                                             VOID *NotifyContext,
                                             EFI_EVENT *Event);
typedef EFI_STATUS(EFIAPI *EFI_SET_TIMER)(EFI_EVENT Event,
                                          EFI_TIMER_DELAY Type,
                                          UINT64 TriggerTime);
typedef EFI_STATUS(EFIAPI *EFI_CLOSE_EVENT)(EFI_EVENT Event);
typedef EFI_STATUS(EFIAPI *EFI_REGISTER_PROTOCOL_NOTIFY)(EFI_GUID *Protocol,
                                                         EFI_EVENT Event,
                                                         VOID **Registration);
typedef EFI_STATUS(EFIAPI *EFI_CREATE_EVENT_EX)(UINT32 Type, EFI_TPL NotifyTpl,
                                                EFI_EVENT_NOTIFY NotifyFunction,
                                                const VOID *NotifyContext,
                                                const EFI_GUID *EventGroup,
                                                EFI_EVENT *Event);
typedef EFI_STATUS(EFIAPI *EFI_SET_VARIABLE)(CHAR16 *VariableName,
                                             EFI_GUID *VendorGuid,
                                             UINT32 Attributes,
                                             UINTN DataSize, VOID *Data);
typedef EFI_STATUS(EFIAPI *EFI_SMM_SET_VARIABLE)(CHAR16 *VariableName,
                                                 EFI_GUID *VendorGuid,
                                                 UINT32 Attributes,
                                                 UINTN DataSize, VOID *Data);

#define EVT_TIMER 0x80000000U
#define EVT_NOTIFY_SIGNAL 0x00000100U
#define TPL_CALLBACK 8U

struct EFI_BOOT_SERVICES {
  EFI_TABLE_HEADER Hdr;
  VOID *RaiseTPL;
  VOID *RestoreTPL;
  EFI_ALLOCATE_PAGES AllocatePages;
  VOID *FreePages;
  VOID *GetMemoryMap;
  VOID *AllocatePool;
  VOID *FreePool;
  EFI_CREATE_EVENT CreateEvent;
  EFI_SET_TIMER SetTimer;
  VOID *WaitForEvent;
  VOID *SignalEvent;
  EFI_CLOSE_EVENT CloseEvent;
  VOID *CheckEvent;
  VOID *InstallProtocolInterface;
  VOID *ReinstallProtocolInterface;
  VOID *UninstallProtocolInterface;
  VOID *HandleProtocol;
  VOID *Reserved;
  EFI_REGISTER_PROTOCOL_NOTIFY RegisterProtocolNotify;
  VOID *LocateHandle;
  VOID *LocateDevicePath;
  EFI_INSTALL_CONFIGURATION_TABLE InstallConfigurationTable;
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
  VOID *LocateHandleBuffer;
  EFI_LOCATE_PROTOCOL LocateProtocol;
  VOID *InstallMultipleProtocolInterfaces;
  VOID *UninstallMultipleProtocolInterfaces;
  VOID *CalculateCrc32;
  VOID *CopyMem;
  VOID *SetMem;
  EFI_CREATE_EVENT_EX CreateEventEx;
};

struct EFI_RUNTIME_SERVICES {
  EFI_TABLE_HEADER Hdr;
  VOID *GetTime;
  VOID *SetTime;
  VOID *GetWakeupTime;
  VOID *SetWakeupTime;
  VOID *SetVirtualAddressMap;
  VOID *ConvertPointer;
  VOID *GetVariable;
  VOID *GetNextVariableName;
  EFI_SET_VARIABLE SetVariable;
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
  EFI_RUNTIME_SERVICES *RuntimeServices;
  EFI_BOOT_SERVICES *BootServices;
  UINTN NumberOfTableEntries;
  EFI_CONFIGURATION_TABLE *ConfigurationTable;
};

typedef EFI_STATUS(EFIAPI *EFI_INSTALL_ACPI_TABLE)(
    const EFI_ACPI_TABLE_PROTOCOL *This, VOID *AcpiTableBuffer,
    UINTN AcpiTableBufferSize, UINTN *TableKey);
typedef EFI_STATUS(EFIAPI *EFI_UNINSTALL_ACPI_TABLE)(
    const EFI_ACPI_TABLE_PROTOCOL *This, UINTN TableKey);

struct EFI_ACPI_TABLE_PROTOCOL {
  EFI_INSTALL_ACPI_TABLE InstallAcpiTable;
  EFI_UNINSTALL_ACPI_TABLE UninstallAcpiTable;
};

typedef EFI_STATUS(EFIAPI *EFI_SMM_COMMUNICATE)(
    const EFI_SMM_COMMUNICATION_PROTOCOL *This, VOID *CommBuffer,
    UINTN *CommSize);

struct EFI_SMM_COMMUNICATION_PROTOCOL {
  EFI_SMM_COMMUNICATE Communicate;
};

#pragma pack(push, 1)
typedef struct {
  EFI_GUID HeaderGuid;
  UINTN MessageLength;
  UINT8 Data[1];
} EFI_SMM_COMMUNICATE_HEADER;

typedef struct {
  UINT64 Magic;
  UINT32 Command;
  UINT32 DataSize;
  UINT64 Sequence;
  UINT64 Arg1;
  UINT64 Arg2;
  UINT64 Arg3;
  UINT8 Data[1];
} REQUEST;

typedef struct {
  UINT64 Magic;
  UINT32 Status;
  UINT32 Command;
  UINT32 DataSize;
  UINT64 Sequence;
  UINT64 Result;
  UINT8 Data[RESPONSE_DATA_SIZE];
} RESPONSE;

typedef struct {
  UINT32 Pid;
  UINT32 Reserved;
  UINT64 Eprocess;
  UINT64 Cr3;
  UINT64 ImageBase;
  char Name[NAME_SIZE];
} PROCESS_INFO;

typedef struct {
  UINT32 Pid;
  UINT32 Reserved;
  UINT64 Base;
  UINT64 Size;
  UINT64 Cr3;
  char Name[NAME_SIZE];
} MODULE_INFO;

typedef struct {
  UINT64 Magic;
  UINT64 MailboxPhysical;
  UINT32 MailboxSize;
  UINT32 SwSmiValue;
} CONFIG;

typedef struct {
  UINT64 Magic;
  UINT32 DxeStage;
  UINT32 DxeStatus;
  UINT32 SmmStage;
  UINT32 SmmStatus;
  UINT64 MailboxPhysical;
  UINT32 MailboxSize;
  UINT32 SwSmiValue;
  UINT32 WmiInstalled;
  UINT32 SmmConfigured;
  UINT32 ConfigureAttempts;
  UINT32 Reserved;
} DEBUG_STATE;

typedef struct {
  UINT32 Stage;
  UINT32 Reserved;
  UINT64 Status;
  UINT64 Data0;
  UINT64 Data1;
  UINT64 Data2;
} DEBUG_RECORD;

typedef struct {
  UINT64 Magic;
  char Build[32];
  UINT32 RecordCount;
  UINT32 LastStage;
  UINT32 LastStatus;
  DEBUG_STATE State;
  DEBUG_RECORD Records[DEBUG_RECORD_COUNT];
} DEBUG_TRACE;
#pragma pack(pop)

typedef EFI_STATUS(EFIAPI *EFI_SMM_HANDLER_ENTRY_POINT2)(
    EFI_HANDLE DispatchHandle, const VOID *Context, VOID *CommBuffer,
    UINTN *CommBufferSize);
typedef EFI_STATUS(EFIAPI *EFI_SMM_INTERRUPT_REGISTER)(
    EFI_SMM_HANDLER_ENTRY_POINT2 Handler, const EFI_GUID *HandlerType,
    EFI_HANDLE *DispatchHandle);
typedef EFI_STATUS(EFIAPI *EFI_SMM_INSIDE_OUT2)(
    const EFI_SMM_BASE2_PROTOCOL *This, BOOLEAN *InSmram);
typedef EFI_STATUS(EFIAPI *EFI_SMM_GET_SMST_LOCATION2)(
    const EFI_SMM_BASE2_PROTOCOL *This, EFI_SMM_SYSTEM_TABLE2 **Smst);

struct EFI_SMM_BASE2_PROTOCOL {
  EFI_SMM_INSIDE_OUT2 InSmm;
  EFI_SMM_GET_SMST_LOCATION2 GetSmstLocation;
};

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

struct EFI_SMM_VARIABLE_PROTOCOL {
  VOID *SmmGetVariable;
  VOID *SmmGetNextVariableName;
  EFI_SMM_SET_VARIABLE SmmSetVariable;
  VOID *SmmQueryVariableInfo;
  VOID *SmmVariableReadyToBoot;
  VOID *SmmVariableExitBootServices;
  VOID *SmmVariableLock;
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
  UINT16 *SmmFirmwareVendor;
  UINT32 SmmFirmwareRevision;
  VOID *SmmInstallConfigurationTable;
  EFI_SMM_CPU_IO2_PROTOCOL SmmIo;
  VOID *SmmAllocatePool;
  VOID *SmmFreePool;
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
  VOID *SmiHandlerUnRegister;
};

static EFI_GUID gConfigCommGuid = {
    0x7cde8c32,
    0x4a4b,
    0x4fa7,
    {0x91, 0x89, 0x2f, 0x35, 0x95, 0x23, 0x11, 0x80}};
static EFI_GUID gWmiGuid = {
    0xa0c9f8de,
    0x0b71,
    0x42a8,
    {0xb9, 0x67, 0xe5, 0x38, 0xea, 0xcb, 0x6f, 0x21}};
static EFI_GUID gConfigGuid = {
    0x3d171a47,
    0x0fa8,
    0x4c86,
    {0x9d, 0x54, 0x54, 0x2d, 0x5c, 0x9d, 0x90, 0x68}};
static EFI_GUID gDebugGuid = {
    0x8ef7c961,
    0x13f3,
    0x4574,
    {0xb4, 0x17, 0x7d, 0x99, 0xa1, 0xa5, 0x2a, 0x8d}};
static EFI_GUID gEfiSmmBase2ProtocolGuid = {
    0xf4ccbfb7,
    0xf6e0,
    0x47fd,
    {0x9d, 0xd4, 0x10, 0xa8, 0xf1, 0x50, 0xc1, 0x91}};
static EFI_GUID gEfiSmmSwDispatch2ProtocolGuid = {
    0x18a3c6dc,
    0x5eea,
    0x48c8,
    {0xa1, 0xc1, 0xb5, 0x33, 0x89, 0xf9, 0x89, 0x99}};
static EFI_GUID gEfiSmmVariableProtocolGuid = {
    0xed32d533,
    0x99e6,
    0x4209,
    {0x9c, 0xc0, 0x2d, 0x72, 0xcd, 0xd9, 0x98, 0xa7}};
static EFI_GUID gEfiSmmCommunicationProtocolGuid = {
    0xc68ed8e2,
    0x9dc6,
    0x4cbd,
    {0x9d, 0x94, 0xdb, 0x65, 0xac, 0xc5, 0xc3, 0x32}};
static EFI_GUID gEfiAcpiTableProtocolGuid = {
    0xffe06bdd,
    0x6107,
    0x46a6,
    {0x7b, 0xb2, 0x5a, 0x9c, 0x7e, 0xc5, 0x27, 0x5c}};

#endif
