#ifndef PAYLOAD_ABI_H
#define PAYLOAD_ABI_H

typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef unsigned long long UINT64;
typedef unsigned long long UINTN;
typedef unsigned long long size_t;
typedef void VOID;
typedef VOID *EFI_HANDLE;
typedef UINT64 EFI_STATUS;

#define EFIAPI __cdecl
#define EFI_SUCCESS 0

#define REASON_LOAD 1U
#define REASON_UNLOAD 2U
#define REASON_DOORBELL 3U

typedef struct {
  UINT32 Data1;
  UINT16 Data2;
  UINT16 Data3;
  UINT8 Data4[8];
} EFI_GUID;

typedef struct EFI_SYSTEM_TABLE EFI_SYSTEM_TABLE;
typedef struct EFI_SMM_SYSTEM_TABLE2 EFI_SMM_SYSTEM_TABLE2;

typedef EFI_STATUS(EFIAPI *EFI_SMM_HANDLER_ENTRY_POINT2)(
    EFI_HANDLE DispatchHandle, const VOID *Context, VOID *CommBuffer,
    UINTN *CommBufferSize);
typedef EFI_STATUS(EFIAPI *EFI_SMM_INTERRUPT_REGISTER)(
    EFI_SMM_HANDLER_ENTRY_POINT2 Handler, const EFI_GUID *HandlerType,
    EFI_HANDLE *DispatchHandle);
typedef EFI_STATUS(EFIAPI *EFI_SMM_INTERRUPT_UNREGISTER)(
    EFI_HANDLE DispatchHandle);
typedef VOID(EFIAPI *SERIAL_PRINT)(const char *Text);
typedef VOID(EFIAPI *SERIAL_HEX64)(UINT64 Value);
typedef EFI_STATUS(EFIAPI *REGISTER_SMI_HANDLER)(
    EFI_SMM_HANDLER_ENTRY_POINT2 Handler, const EFI_GUID *HandlerType,
    EFI_HANDLE *DispatchHandle);
typedef EFI_STATUS(EFIAPI *UNREGISTER_SMI_HANDLER)(
    EFI_HANDLE DispatchHandle);
typedef EFI_STATUS(EFIAPI *ALLOCATE_POOL)(UINTN Size, VOID **Buffer);
typedef EFI_STATUS(EFIAPI *FREE_POOL)(VOID *Buffer);

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

typedef EFI_STATUS(EFIAPI *PAYLOAD_ENTRY)(
    PAYLOAD_CONTEXT *Context);

#endif