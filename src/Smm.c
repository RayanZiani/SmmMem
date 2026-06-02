#include <intrin.h>
#include "Common.h"

#pragma intrinsic(__inbyte)
#pragma intrinsic(__outbyte)
#pragma intrinsic(__readcr3)
#pragma intrinsic(__readmsr)
#pragma intrinsic(__cpuidex)
#pragma function(memset)
#pragma function(memcpy)

#define PAGE_SIZE 0x1000ULL
#define PAGE_MASK 0xFFFFFFFFFFFFF000ULL
#define LARGE_PAGE_SIZE 0x200000ULL
#define LARGE_PAGE_MASK 0xFFFFFFFFFFE00000ULL
#define PTE_PRESENT 1ULL
#define PTE_LARGE (1ULL << 7)
#define MSR_LSTAR 0xC0000082U
#define SAVE_STATE_CR3 53U

typedef struct EFI_SMM_CPU_PROTOCOL EFI_SMM_CPU_PROTOCOL;
typedef EFI_STATUS(EFIAPI *READ_SAVE_STATE)(const EFI_SMM_CPU_PROTOCOL *This,
                                            UINTN Width, UINT32 Register,
                                            UINTN CpuIndex, VOID *Buffer);
typedef EFI_STATUS(EFIAPI *WRITE_SAVE_STATE)(const EFI_SMM_CPU_PROTOCOL *This,
                                             UINTN Width, UINT32 Register,
                                             UINTN CpuIndex,
                                             const VOID *Buffer);
typedef EFI_STATUS(EFIAPI *SMM_CPU_IO)(const EFI_SMM_CPU_IO2_PROTOCOL *This,
                                       UINT32 Width, UINT64 Address,
                                       UINTN Count, VOID *Buffer);

struct EFI_SMM_CPU_PROTOCOL {
  READ_SAVE_STATE ReadSaveState;
  WRITE_SAVE_STATE WriteSaveState;
};

static EFI_GUID gEfiSmmCpuProtocolGuid = {
    0xeb346b97,
    0x975f,
    0x4a9f,
    {0x8b, 0x22, 0xf8, 0xe9, 0x2b, 0xb3, 0xd5, 0x69}};

static EFI_SYSTEM_TABLE *gSystemTable;
static EFI_SMM_SYSTEM_TABLE2 *gSmst;
static EFI_SMM_CPU_PROTOCOL *gSmmCpu;
static EFI_PHYSICAL_ADDRESS gMailboxPhysical;
static UINT32 gMailboxSize;
static UINT32 gSwSmiValue = SW_SMI_VALUE;
static EFI_HANDLE gCommHandle;
static EFI_HANDLE gSwHandle;
static UINT64 gKernelCr3;
static UINT64 gKernelBase;
static UINT64 gSystemProcess;
static UINT64 gPsLoadedModuleList;
static UINT32 gPidOffset;
static UINT32 gLinksOffset;
static UINT32 gNameOffset;
static UINT32 gPebOffset;
static UINT32 gCr3Offset;
static UINT64 gPhysMask;

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

static VOID WriteText(char *Out, UINTN OutSize, const char *Text) {
  UINTN Index;
  if (OutSize == 0) {
    return;
  }
  for (Index = 0; Index + 1 < OutSize && Text[Index] != 0; Index++) {
    Out[Index] = Text[Index];
  }
  Out[Index] = 0;
}

static char Lower(char Ch) {
  if (Ch >= 'A' && Ch <= 'Z') {
    return (char)(Ch + ('a' - 'A'));
  }
  return Ch;
}

static BOOLEAN SameName(const char *A, const char *B) {
  while (*A != 0 && *B != 0) {
    if (Lower(*A++) != Lower(*B++)) {
      return 0;
    }
  }
  return *A == 0 && *B == 0;
}

static BOOLEAN IsKernelPtr(UINT64 Value) {
  return Value >= 0xFFFF800000000000ULL;
}

static BOOLEAN IsUserPtr(UINT64 Value) {
  return Value >= 0x10000ULL && Value < 0x0000800000000000ULL;
}

static EFI_STATUS CopyPhys(UINT64 Address, VOID *Buffer, UINTN Size,
                           BOOLEAN Write) {
  SMM_CPU_IO Access;

  Access = (SMM_CPU_IO)(Write ? gSmst->SmmIo.Mem.Write
                              : gSmst->SmmIo.Mem.Read);
  return Access(&gSmst->SmmIo, 0, Address, Size, Buffer);
}

static UINT64 PhysMask(VOID) {
  int Regs[4];
  UINT32 PhysBits;
  UINT32 CBit;

  if (gPhysMask != 0) {
    return gPhysMask;
  }
  __cpuidex(Regs, 0x80000008, 0);
  PhysBits = (UINT32)(Regs[0] & 0xFF);
  if (PhysBits == 0 || PhysBits > 52) {
    PhysBits = 52;
  }
  gPhysMask = ((1ULL << PhysBits) - 1ULL) & PAGE_MASK;
  __cpuidex(Regs, 0x8000001F, 0);
  if ((Regs[0] & 1) != 0) {
    CBit = (UINT32)(Regs[1] & 0x3F);
    if (CBit < 63) {
      gPhysMask &= ~(1ULL << CBit);
    }
  }
  return gPhysMask;
}

static UINT64 PteAddress(UINT64 Entry) {
  return Entry & PhysMask();
}

static EFI_STATUS ReadPhys64(UINT64 Address, UINT64 *Value) {
  return CopyPhys(Address, Value, sizeof(*Value), 0);
}

static EFI_STATUS TranslateCr3(UINT64 Cr3, UINT64 Va, UINT64 *Pa) {
  UINT64 Entry;
  UINT64 Base = Cr3 & PhysMask();

  if (ReadPhys64(Base + (((Va >> 39) & 0x1FF) * 8), &Entry) != EFI_SUCCESS ||
      (Entry & PTE_PRESENT) == 0) {
    return EFI_NOT_FOUND;
  }
  Base = PteAddress(Entry);
  if (ReadPhys64(Base + (((Va >> 30) & 0x1FF) * 8), &Entry) != EFI_SUCCESS ||
      (Entry & PTE_PRESENT) == 0) {
    return EFI_NOT_FOUND;
  }
  if ((Entry & PTE_LARGE) != 0) {
    *Pa = (Entry & PhysMask() & 0x000FFFFFC0000000ULL) +
          (Va & 0x3FFFFFFFULL);
    return EFI_SUCCESS;
  }
  Base = PteAddress(Entry);
  if (ReadPhys64(Base + (((Va >> 21) & 0x1FF) * 8), &Entry) != EFI_SUCCESS ||
      (Entry & PTE_PRESENT) == 0) {
    return EFI_NOT_FOUND;
  }
  if ((Entry & PTE_LARGE) != 0) {
    *Pa = (Entry & PhysMask() & 0x000FFFFFFFE00000ULL) +
          (Va & 0x1FFFFFULL);
    return EFI_SUCCESS;
  }
  Base = PteAddress(Entry);
  if (ReadPhys64(Base + (((Va >> 12) & 0x1FF) * 8), &Entry) != EFI_SUCCESS ||
      (Entry & PTE_PRESENT) == 0) {
    return EFI_NOT_FOUND;
  }
  *Pa = PteAddress(Entry) + (Va & 0xFFFULL);
  return EFI_SUCCESS;
}

static EFI_STATUS CopyVirtCr3(UINT64 Cr3, UINT64 Va, VOID *Buffer,
                              UINTN Size, BOOLEAN Write) {
  UINT8 *Bytes = (UINT8 *)Buffer;
  while (Size != 0) {
    UINT64 Pa;
    UINTN Chunk = (UINTN)(PAGE_SIZE - (Va & (PAGE_SIZE - 1)));
    if (Chunk > Size) {
      Chunk = Size;
    }
    if (EFI_ERROR(TranslateCr3(Cr3, Va, &Pa))) {
      return EFI_NOT_FOUND;
    }
    if (EFI_ERROR(CopyPhys(Pa, Bytes, Chunk, Write))) {
      return EFI_NOT_FOUND;
    }
    Va += Chunk;
    Bytes += Chunk;
    Size -= Chunk;
  }
  return EFI_SUCCESS;
}

static EFI_STATUS ReadVirt64(UINT64 Cr3, UINT64 Va, UINT64 *Value) {
  return CopyVirtCr3(Cr3, Va, Value, sizeof(*Value), 0);
}

static EFI_STATUS ReadVirt32(UINT64 Cr3, UINT64 Va, UINT32 *Value) {
  return CopyVirtCr3(Cr3, Va, Value, sizeof(*Value), 0);
}

static EFI_STATUS ReadVirt16(UINT64 Cr3, UINT64 Va, UINT16 *Value) {
  return CopyVirtCr3(Cr3, Va, Value, sizeof(*Value), 0);
}

static EFI_STATUS ReadAscii(UINT64 Cr3, UINT64 Va, char *Out, UINTN OutSize) {
  UINTN Index;
  if (OutSize == 0) {
    return EFI_INVALID_PARAMETER;
  }
  for (Index = 0; Index + 1 < OutSize; Index++) {
    if (CopyVirtCr3(Cr3, Va + Index, &Out[Index], 1, 0) != EFI_SUCCESS) {
      return EFI_NOT_FOUND;
    }
    if (Out[Index] == 0) {
      return EFI_SUCCESS;
    }
  }
  Out[Index] = 0;
  return EFI_SUCCESS;
}

static EFI_STATUS ReadUnicodeName(UINT64 Cr3, UINT64 StringAddress,
                                  char *Out, UINTN OutSize) {
  UINT16 Length;
  UINT64 Buffer;
  UINTN Count;
  UINTN Index;

  if (ReadVirt16(Cr3, StringAddress, &Length) != EFI_SUCCESS ||
      ReadVirt64(Cr3, StringAddress + 8, &Buffer) != EFI_SUCCESS ||
      Buffer == 0 || OutSize == 0) {
    return EFI_NOT_FOUND;
  }
  Count = Length / 2;
  if (Count + 1 > OutSize) {
    Count = OutSize - 1;
  }
  for (Index = 0; Index < Count; Index++) {
    UINT16 Ch;
    if (ReadVirt16(Cr3, Buffer + Index * 2, &Ch) != EFI_SUCCESS) {
      return EFI_NOT_FOUND;
    }
    Out[Index] = (char)(Ch & 0xFF);
  }
  Out[Index] = 0;
  return EFI_SUCCESS;
}

static EFI_STATUS ReadLdrEntry(UINT64 Cr3, UINT64 Link, UINT32 Bias,
                               char *Name, UINTN NameSize, UINT64 *Base,
                               UINT32 *Size) {
  if (ReadVirt64(Cr3, Link + 0x30 - Bias, Base) != EFI_SUCCESS ||
      ReadVirt32(Cr3, Link + 0x40 - Bias, Size) != EFI_SUCCESS ||
      !IsUserPtr(*Base) ||
      ReadUnicodeName(Cr3, Link + 0x58 - Bias, Name, NameSize) !=
          EFI_SUCCESS) {
    return EFI_NOT_FOUND;
  }
  return EFI_SUCCESS;
}

static EFI_STATUS ResolveExport(UINT64 Cr3, UINT64 Base, const char *Name,
                                UINT64 *Address) {
  UINT16 Mz;
  UINT32 Pe;
  UINT32 Sig;
  UINT16 Magic;
  UINT64 DirOffset;
  UINT32 ExportRva;
  UINT32 Names;
  UINT32 FunctionsRva;
  UINT32 NamesRva;
  UINT32 OrdinalsRva;
  UINT32 Index;

  if (ReadVirt16(Cr3, Base, &Mz) != EFI_SUCCESS || Mz != 0x5A4D ||
      ReadVirt32(Cr3, Base + 0x3C, &Pe) != EFI_SUCCESS ||
      ReadVirt32(Cr3, Base + Pe, &Sig) != EFI_SUCCESS ||
      Sig != 0x00004550 ||
      ReadVirt16(Cr3, Base + Pe + 0x18, &Magic) != EFI_SUCCESS) {
    return EFI_NOT_FOUND;
  }
  DirOffset = Base + Pe + 0x18 + (Magic == 0x20B ? 0x70 : 0x60);
  if (ReadVirt32(Cr3, DirOffset, &ExportRva) != EFI_SUCCESS ||
      ExportRva == 0 ||
      ReadVirt32(Cr3, Base + ExportRva + 0x18, &Names) != EFI_SUCCESS ||
      ReadVirt32(Cr3, Base + ExportRva + 0x1C, &FunctionsRva) != EFI_SUCCESS ||
      ReadVirt32(Cr3, Base + ExportRva + 0x20, &NamesRva) != EFI_SUCCESS ||
      ReadVirt32(Cr3, Base + ExportRva + 0x24, &OrdinalsRva) != EFI_SUCCESS) {
    return EFI_NOT_FOUND;
  }
  if (Names > 65536) {
    Names = 65536;
  }
  for (Index = 0; Index < Names; Index++) {
    UINT32 NameRva;
    UINT16 Ordinal;
    UINT32 FunctionRva;
    char ExportName[128];
    if (ReadVirt32(Cr3, Base + NamesRva + Index * 4, &NameRva) != EFI_SUCCESS ||
        ReadAscii(Cr3, Base + NameRva, ExportName, sizeof(ExportName)) !=
            EFI_SUCCESS) {
      continue;
    }
    if (!SameName(ExportName, Name)) {
      continue;
    }
    if (ReadVirt16(Cr3, Base + OrdinalsRva + Index * 2, &Ordinal) !=
            EFI_SUCCESS ||
        ReadVirt32(Cr3, Base + FunctionsRva + Ordinal * 4, &FunctionRva) !=
            EFI_SUCCESS) {
      return EFI_NOT_FOUND;
    }
    *Address = Base + FunctionRva;
    return EFI_SUCCESS;
  }
  return EFI_NOT_FOUND;
}

static VOID LocateSmmCpu(VOID) {
  EFI_LOCATE_PROTOCOL Locate;

  if (gSmmCpu == 0) {
    Locate = (EFI_LOCATE_PROTOCOL)gSmst->SmmLocateProtocol;
    Locate(&gEfiSmmCpuProtocolGuid, 0, (VOID **)&gSmmCpu);
  }
}

static EFI_STATUS ReadSavedCr3(UINTN CpuIndex, UINT64 *Cr3) {
  LocateSmmCpu();
  if (gSmmCpu != 0 && gSmmCpu->ReadSaveState != 0 &&
      gSmmCpu->ReadSaveState(gSmmCpu, sizeof(*Cr3), SAVE_STATE_CR3,
                             CpuIndex, Cr3) == EFI_SUCCESS &&
      *Cr3 != 0) {
    *Cr3 &= PAGE_MASK;
    return EFI_SUCCESS;
  }
  return EFI_NOT_FOUND;
}

static BOOLEAN LooksLikeProcessList(UINT64 Eprocess, UINT32 PidOffset) {
  UINT64 Flink;
  UINT64 Blink;
  UINT64 Back;

  if (ReadVirt64(gKernelCr3, Eprocess + PidOffset + 8, &Flink) !=
          EFI_SUCCESS ||
      ReadVirt64(gKernelCr3, Eprocess + PidOffset + 16, &Blink) !=
          EFI_SUCCESS ||
      !IsKernelPtr(Flink) || !IsKernelPtr(Blink) ||
      ReadVirt64(gKernelCr3, Flink + 8, &Back) != EFI_SUCCESS) {
    return 0;
  }
  return Back == Eprocess + PidOffset + 8;
}

static EFI_STATUS ResolveListLayout(VOID) {
  UINT32 PidOffset;

  if (!IsKernelPtr(gSystemProcess)) {
    return EFI_NOT_FOUND;
  }
  for (PidOffset = 0x20; PidOffset < 0x900; PidOffset += 8) {
    UINT64 Pid;
    if (ReadVirt64(gKernelCr3, gSystemProcess + PidOffset, &Pid) ==
            EFI_SUCCESS &&
        (UINT32)Pid == 4 &&
        LooksLikeProcessList(gSystemProcess, PidOffset)) {
      gPidOffset = PidOffset;
      gLinksOffset = PidOffset + 8;
      return EFI_SUCCESS;
    }
  }
  return EFI_NOT_FOUND;
}

static EFI_STATUS ResolveNameOffset(VOID) {
  UINT32 Offset;

  if (gNameOffset != 0) {
    return EFI_SUCCESS;
  }
  for (Offset = 0x100; Offset < 0x900; Offset++) {
    char Name[NAME_SIZE];
    ZeroMem(Name, sizeof(Name));
    if (CopyVirtCr3(gKernelCr3, gSystemProcess + Offset, Name,
                    sizeof(Name) - 1, 0) == EFI_SUCCESS &&
        SameName(Name, "System")) {
      gNameOffset = Offset;
      return EFI_SUCCESS;
    }
  }
  return EFI_NOT_FOUND;
}

static EFI_STATUS ResolveCr3Offset(VOID) {
  UINT32 Offset;

  if (gCr3Offset != 0) {
    return EFI_SUCCESS;
  }
  for (Offset = 0x20; Offset < 0x100; Offset += 8) {
    UINT64 Cr3;
    UINT16 Mz;
    if (ReadVirt64(gKernelCr3, gSystemProcess + Offset, &Cr3) != EFI_SUCCESS ||
        Cr3 == 0 || (Cr3 & 0xFFFULL) != 0) {
      continue;
    }
    Cr3 &= PAGE_MASK;
    if (ReadVirt16(Cr3, gKernelBase, &Mz) == EFI_SUCCESS && Mz == 0x5A4D) {
      gCr3Offset = Offset;
      return EFI_SUCCESS;
    }
  }
  return EFI_NOT_FOUND;
}

static EFI_STATUS TryKernelCr3(UINT64 Cr3, UINT64 Lstar) {
  UINT64 Start;
  UINT64 Limit;
  UINT64 Base;
  UINT64 Export;

  gKernelCr3 = Cr3 & PAGE_MASK;
  Start = Lstar & LARGE_PAGE_MASK;
  Limit = Start > 0x10000000ULL ? Start - 0x10000000ULL : 0;
  for (Base = Start; Base > Limit; Base -= LARGE_PAGE_SIZE) {
    UINT16 Mz;
    if (ReadVirt16(gKernelCr3, Base, &Mz) != EFI_SUCCESS || Mz != 0x5A4D) {
      continue;
    }
    if (ResolveExport(gKernelCr3, Base, "PsInitialSystemProcess",
                      &Export) == EFI_SUCCESS) {
      gKernelBase = Base;
      if (ReadVirt64(gKernelCr3, Export, &gSystemProcess) != EFI_SUCCESS) {
        return EFI_NOT_FOUND;
      }
      ResolveExport(gKernelCr3, Base, "PsLoadedModuleList",
                    &gPsLoadedModuleList);
      return EFI_SUCCESS;
    }
  }
  return EFI_NOT_FOUND;
}

static EFI_STATUS InitKernel(VOID) {
  UINT64 Lstar;
  UINT64 Cr3;
  UINTN Cpu;

  if (gKernelBase != 0 && gSystemProcess != 0) {
    return EFI_SUCCESS;
  }
  Lstar = __readmsr(MSR_LSTAR);
  for (Cpu = 0; Cpu < gSmst->NumberOfCpus; Cpu++) {
    if (ReadSavedCr3(Cpu, &Cr3) == EFI_SUCCESS && Cr3 < 0x100000000ULL &&
        TryKernelCr3(Cr3, Lstar) == EFI_SUCCESS) {
      return EFI_SUCCESS;
    }
  }
  for (Cpu = 0; Cpu < gSmst->NumberOfCpus; Cpu++) {
    if (ReadSavedCr3(Cpu, &Cr3) == EFI_SUCCESS &&
        TryKernelCr3(Cr3, Lstar) == EFI_SUCCESS) {
      return EFI_SUCCESS;
    }
  }
  Cr3 = __readcr3() & PAGE_MASK;
  return TryKernelCr3(Cr3, Lstar);
}

static EFI_STATUS ResolveProcessLayout(VOID) {
  if (gPidOffset != 0 && gLinksOffset != 0 && gCr3Offset != 0) {
    return EFI_SUCCESS;
  }
  if (InitKernel() != EFI_SUCCESS || ResolveListLayout() != EFI_SUCCESS ||
      ResolveCr3Offset() != EFI_SUCCESS) {
    return EFI_NOT_FOUND;
  }
  ResolveNameOffset();
  return EFI_SUCCESS;
}

static EFI_STATUS GetProcessCr3(UINT64 Eprocess, UINT64 *Cr3) {
  if (gCr3Offset == 0 ||
      ReadVirt64(gKernelCr3, Eprocess + gCr3Offset, Cr3) != EFI_SUCCESS ||
      *Cr3 == 0) {
    return EFI_NOT_FOUND;
  }
  *Cr3 &= PAGE_MASK;
  return *Cr3 != 0 ? EFI_SUCCESS : EFI_NOT_FOUND;
}

static EFI_STATUS FindPeb(UINT64 Eprocess, UINT64 Cr3, UINT64 *Peb) {
  UINT32 Offset;
  UINT32 ListOffsets[] = {0x10, 0x20, 0x30};
  UINT32 Biases[] = {0x00, 0x10, 0x20};
  UINTN Index;

  if (gPebOffset != 0 &&
      ReadVirt64(gKernelCr3, Eprocess + gPebOffset, Peb) == EFI_SUCCESS &&
      IsUserPtr(*Peb)) {
    return EFI_SUCCESS;
  }
  for (Offset = 0x100; Offset < 0x900; Offset += 8) {
    UINT64 Candidate;
    UINT64 Ldr;
    if (ReadVirt64(gKernelCr3, Eprocess + Offset, &Candidate) != EFI_SUCCESS ||
        !IsUserPtr(Candidate) ||
        ReadVirt64(Cr3, Candidate + 0x18, &Ldr) != EFI_SUCCESS ||
        !IsUserPtr(Ldr)) {
      continue;
    }
    for (Index = 0; Index < sizeof(ListOffsets) / sizeof(ListOffsets[0]);
         Index++) {
      UINT64 Head = Ldr + ListOffsets[Index];
      UINT64 Link;
      UINT64 Base;
      UINT16 Mz;
      if (ReadVirt64(Cr3, Head, &Link) == EFI_SUCCESS && IsUserPtr(Link) &&
          Link != Head &&
          ReadVirt64(Cr3, Link + 0x30 - Biases[Index], &Base) ==
              EFI_SUCCESS &&
          IsUserPtr(Base) &&
          ReadVirt16(Cr3, Base, &Mz) == EFI_SUCCESS && Mz == 0x5A4D) {
        gPebOffset = Offset;
        *Peb = Candidate;
        return EFI_SUCCESS;
      }
    }
  }
  return EFI_NOT_FOUND;
}

static EFI_STATUS FindImageBase(UINT64 Eprocess, UINT64 Cr3, UINT64 *Base) {
  UINT64 Peb;
  UINT64 Ldr;
  UINT32 ListOffsets[] = {0x10, 0x20, 0x30};
  UINT32 Biases[] = {0x00, 0x10, 0x20};
  UINTN Index;
  if (FindPeb(Eprocess, Cr3, &Peb) == EFI_SUCCESS &&
      ReadVirt64(Cr3, Peb + 0x18, &Ldr) == EFI_SUCCESS && IsUserPtr(Ldr)) {
    for (Index = 0; Index < sizeof(ListOffsets) / sizeof(ListOffsets[0]);
         Index++) {
      UINT64 Head = Ldr + ListOffsets[Index];
      UINT64 Link;
      if (ReadVirt64(Cr3, Head, &Link) == EFI_SUCCESS && IsUserPtr(Link) &&
          Link != Head &&
          ReadVirt64(Cr3, Link + 0x30 - Biases[Index], Base) == EFI_SUCCESS &&
          IsUserPtr(*Base)) {
        return EFI_SUCCESS;
      }
    }
  }
  *Base = 0;
  return EFI_NOT_FOUND;
}

static EFI_STATUS FillProcessInfo(UINT64 Eprocess, PROCESS_INFO *Info) {
  UINT64 Pid;
  UINT64 Cr3;
  ZeroMem(Info, sizeof(*Info));
  if (ReadVirt64(gKernelCr3, Eprocess + gPidOffset, &Pid) != EFI_SUCCESS ||
      GetProcessCr3(Eprocess, &Cr3) != EFI_SUCCESS) {
    return EFI_NOT_FOUND;
  }
  Info->Pid = (UINT32)Pid;
  Info->Eprocess = Eprocess;
  Info->Cr3 = Cr3;
  if (gNameOffset != 0) {
    CopyVirtCr3(gKernelCr3, Eprocess + gNameOffset, Info->Name,
                sizeof(Info->Name) - 1, 0);
    Info->Name[sizeof(Info->Name) - 1] = 0;
  }
  FindImageBase(Eprocess, Cr3, &Info->ImageBase);
  return EFI_SUCCESS;
}

static EFI_STATUS FindProcessPid(UINT32 Pid, PROCESS_INFO *Info) {
  UINT64 Head;
  UINT64 Link;
  UINT32 Guard;

  if (ResolveProcessLayout() != EFI_SUCCESS) {
    return EFI_NOT_FOUND;
  }
  if (Pid == 4) {
    return FillProcessInfo(gSystemProcess, Info);
  }
  Head = gSystemProcess + gLinksOffset;
  if (ReadVirt64(gKernelCr3, Head, &Link) != EFI_SUCCESS) {
    return EFI_NOT_FOUND;
  }
  for (Guard = 0; Guard < 4096 && IsKernelPtr(Link) && Link != Head; Guard++) {
    UINT64 Eprocess = Link - gLinksOffset;
    UINT64 CurrentPid;
    if (ReadVirt64(gKernelCr3, Eprocess + gPidOffset, &CurrentPid) !=
        EFI_SUCCESS) {
      break;
    }
    if ((UINT32)CurrentPid == Pid) {
      return FillProcessInfo(Eprocess, Info);
    }
    if (ReadVirt64(gKernelCr3, Eprocess + gLinksOffset, &Link) !=
        EFI_SUCCESS) {
      break;
    }
  }
  return EFI_NOT_FOUND;
}

static EFI_STATUS FindProcessName(const char *Name, PROCESS_INFO *Info) {
  UINT64 Head;
  UINT64 Link;
  UINT32 Guard;
  char CurrentName[NAME_SIZE];

  if (ResolveProcessLayout() != EFI_SUCCESS || gNameOffset == 0) {
    return EFI_NOT_FOUND;
  }
  ZeroMem(CurrentName, sizeof(CurrentName));
  CopyVirtCr3(gKernelCr3, gSystemProcess + gNameOffset, CurrentName,
              sizeof(CurrentName) - 1, 0);
  if (SameName(CurrentName, Name)) {
    return FillProcessInfo(gSystemProcess, Info);
  }
  Head = gSystemProcess + gLinksOffset;
  if (ReadVirt64(gKernelCr3, Head, &Link) != EFI_SUCCESS) {
    return EFI_NOT_FOUND;
  }
  for (Guard = 0; Guard < 4096 && IsKernelPtr(Link) && Link != Head; Guard++) {
    UINT64 Eprocess = Link - gLinksOffset;
    ZeroMem(CurrentName, sizeof(CurrentName));
    CopyVirtCr3(gKernelCr3, Eprocess + gNameOffset, CurrentName,
                sizeof(CurrentName) - 1, 0);
    if (SameName(CurrentName, Name)) {
      return FillProcessInfo(Eprocess, Info);
    }
    if (ReadVirt64(gKernelCr3, Eprocess + gLinksOffset, &Link) !=
        EFI_SUCCESS) {
      break;
    }
  }
  return EFI_NOT_FOUND;
}

static EFI_STATUS FindUserModule(UINT32 Pid, const char *Name,
                                 MODULE_INFO *Module) {
  PROCESS_INFO Process;
  UINT64 Peb;
  UINT64 Ldr;
  UINT32 ListOffsets[] = {0x10, 0x20, 0x30};
  UINT32 Biases[] = {0x00, 0x10, 0x20};
  UINT32 Guard;
  UINTN Index;

  if (FindProcessPid(Pid, &Process) != EFI_SUCCESS ||
      FindPeb(Process.Eprocess, Process.Cr3, &Peb) != EFI_SUCCESS ||
      ReadVirt64(Process.Cr3, Peb + 0x18, &Ldr) != EFI_SUCCESS ||
      !IsUserPtr(Ldr)) {
    return EFI_NOT_FOUND;
  }
  for (Index = 0; Index < sizeof(ListOffsets) / sizeof(ListOffsets[0]);
       Index++) {
    UINT64 Head = Ldr + ListOffsets[Index];
    UINT64 Link;
    if (ReadVirt64(Process.Cr3, Head, &Link) != EFI_SUCCESS) {
      continue;
    }
    for (Guard = 0; Guard < 1024 && IsUserPtr(Link) && Link != Head; Guard++) {
      char ModuleName[NAME_SIZE];
      UINT64 Base;
      UINT32 Size;
      ZeroMem(ModuleName, sizeof(ModuleName));
      if (ReadLdrEntry(Process.Cr3, Link, Biases[Index], ModuleName,
                       sizeof(ModuleName), &Base, &Size) == EFI_SUCCESS &&
          (Name[0] == 0 || SameName(ModuleName, Name))) {
        ZeroMem(Module, sizeof(*Module));
        Module->Pid = Pid;
        Module->Base = Base;
        Module->Size = Size;
        Module->Cr3 = Process.Cr3;
        WriteText(Module->Name, sizeof(Module->Name), ModuleName);
        return EFI_SUCCESS;
      }
      if (ReadVirt64(Process.Cr3, Link, &Link) != EFI_SUCCESS) {
        break;
      }
    }
  }
  return EFI_NOT_FOUND;
}

static EFI_STATUS FindKernelModule(const char *Name, MODULE_INFO *Module) {
  UINT64 Head;
  UINT64 Link;
  UINT32 Guard;

  if (InitKernel() != EFI_SUCCESS || gPsLoadedModuleList == 0) {
    return EFI_NOT_FOUND;
  }
  Head = gPsLoadedModuleList;
  if (ReadVirt64(gKernelCr3, Head, &Link) != EFI_SUCCESS) {
    return EFI_NOT_FOUND;
  }
  for (Guard = 0; Guard < 1024 && IsKernelPtr(Link) && Link != Head; Guard++) {
    char ModuleName[NAME_SIZE];
    UINT64 Base;
    UINT32 Size;
    if (ReadVirt64(gKernelCr3, Link + 0x30, &Base) == EFI_SUCCESS &&
        ReadVirt32(gKernelCr3, Link + 0x40, &Size) == EFI_SUCCESS &&
        ReadUnicodeName(gKernelCr3, Link + 0x58, ModuleName,
                        sizeof(ModuleName)) == EFI_SUCCESS &&
        (Name[0] == 0 || SameName(ModuleName, Name))) {
      ZeroMem(Module, sizeof(*Module));
      Module->Pid = 4;
      Module->Base = Base;
      Module->Size = Size;
      Module->Cr3 = gKernelCr3;
      WriteText(Module->Name, sizeof(Module->Name), ModuleName);
      return EFI_SUCCESS;
    }
    if (ReadVirt64(gKernelCr3, Link, &Link) != EFI_SUCCESS) {
      break;
    }
  }
  return EFI_NOT_FOUND;
}

static VOID Reply(RESPONSE *Response, REQUEST *Request, EFI_STATUS Status,
                  UINT64 Result, const VOID *Data, UINT32 DataSize) {
  ZeroMem(Response, sizeof(*Response));
  Response->Magic = RESP_MAGIC;
  Response->Status = (UINT32)Status;
  Response->Command = Request->Command;
  Response->Sequence = Request->Sequence;
  Response->Result = Result;
  if (DataSize > RESPONSE_DATA_SIZE) {
    DataSize = RESPONSE_DATA_SIZE;
  }
  Response->DataSize = DataSize;
  if (Data != 0 && DataSize != 0) {
    CopyMemLocal(Response->Data, Data, DataSize);
  }
}

static EFI_STATUS HandleRequest(REQUEST *Request, RESPONSE *Response) {
  PROCESS_INFO Process;
  MODULE_INFO Module;
  EFI_STATUS Status;
  UINT32 Size;
  UINT64 Pa = 0;
  UINT64 Address = 0;
  UINT8 Scratch[RESPONSE_DATA_SIZE];

  ZeroMem(&Process, sizeof(Process));
  ZeroMem(&Module, sizeof(Module));
  ZeroMem(Scratch, sizeof(Scratch));
  if (Request->Magic != REQ_MAGIC) {
    return EFI_NOT_FOUND;
  }
  Size = (UINT32)Request->Arg3;
  if (Request->Command == CMD_PING) {
    Reply(Response, Request, EFI_SUCCESS, 0x504F4E47ULL, 0, 0);
    return EFI_SUCCESS;
  }
  if (Request->Command == CMD_READ_PHYS) {
    Size = (UINT32)Request->Arg2;
    if (Size > RESPONSE_DATA_SIZE) {
      Reply(Response, Request, EFI_INVALID_PARAMETER, 0, 0, 0);
      return EFI_INVALID_PARAMETER;
    }
    Status = CopyPhys(Request->Arg1, Scratch, Size, 0);
    Reply(Response, Request, Status, Request->Arg1, Scratch,
          EFI_ERROR(Status) ? 0 : Size);
    return Status;
  }
  if (Request->Command == CMD_WRITE_PHYS) {
    Status = Request->DataSize <= RESPONSE_DATA_SIZE
                 ? CopyPhys(Request->Arg1, Request->Data, Request->DataSize, 1)
                 : EFI_INVALID_PARAMETER;
    Reply(Response, Request, Status, Request->Arg1, 0, 0);
    return Status;
  }
  if (Request->Command == CMD_FIND_PROCESS_PID) {
    Status = FindProcessPid((UINT32)Request->Arg1, &Process);
    Reply(Response, Request, Status, Process.Eprocess, &Process,
          EFI_ERROR(Status) ? 0 : sizeof(Process));
    return Status;
  }
  if (Request->Command == CMD_FIND_PROCESS_NAME) {
    Status = FindProcessName((char *)Request->Data, &Process);
    Reply(Response, Request, Status, Process.Eprocess, &Process,
          EFI_ERROR(Status) ? 0 : sizeof(Process));
    return Status;
  }
  if (Request->Command == CMD_TRANSLATE_VIRT) {
    Status = FindProcessPid((UINT32)Request->Arg1, &Process);
    if (!EFI_ERROR(Status)) {
      Status = TranslateCr3(Process.Cr3, Request->Arg2, &Pa);
    }
    Reply(Response, Request, Status, EFI_ERROR(Status) ? 0 : Pa, 0, 0);
    return Status;
  }
  if (Request->Command == CMD_READ_VIRT) {
    if (Size > RESPONSE_DATA_SIZE) {
      Reply(Response, Request, EFI_INVALID_PARAMETER, 0, 0, 0);
      return EFI_INVALID_PARAMETER;
    }
    Status = FindProcessPid((UINT32)Request->Arg1, &Process);
    if (!EFI_ERROR(Status)) {
      Status = CopyVirtCr3(Process.Cr3, Request->Arg2, Scratch, Size, 0);
    }
    Reply(Response, Request, Status, Request->Arg2, Scratch,
          EFI_ERROR(Status) ? 0 : Size);
    return Status;
  }
  if (Request->Command == CMD_WRITE_VIRT) {
    if (Request->DataSize > RESPONSE_DATA_SIZE) {
      Reply(Response, Request, EFI_INVALID_PARAMETER, 0, 0, 0);
      return EFI_INVALID_PARAMETER;
    }
    Status = FindProcessPid((UINT32)Request->Arg1, &Process);
    if (!EFI_ERROR(Status)) {
      Status = CopyVirtCr3(Process.Cr3, Request->Arg2, Request->Data,
                           Request->DataSize, 1);
    }
    Reply(Response, Request, Status, Request->Arg2, 0, 0);
    return Status;
  }
  if (Request->Command == CMD_FIND_MODULE) {
    Status = FindUserModule((UINT32)Request->Arg1, (char *)Request->Data,
                            &Module);
    Reply(Response, Request, Status, Module.Base, &Module,
          EFI_ERROR(Status) ? 0 : sizeof(Module));
    return Status;
  }
  if (Request->Command == CMD_FIND_KERNEL_MODULE) {
    Status = FindKernelModule((char *)Request->Data, &Module);
    Reply(Response, Request, Status, Module.Base, &Module,
          EFI_ERROR(Status) ? 0 : sizeof(Module));
    return Status;
  }
  if (Request->Command == CMD_FIND_EXPORT) {
    if (Request->DataSize <= sizeof(MODULE_INFO)) {
      Reply(Response, Request, EFI_INVALID_PARAMETER, 0, 0, 0);
      return EFI_INVALID_PARAMETER;
    }
    CopyMemLocal(&Module, Request->Data, sizeof(Module));
    Address = 0;
    Status = ResolveExport(Module.Cr3 != 0 ? Module.Cr3 : gKernelCr3,
                           Module.Base,
                           (char *)(Request->Data + sizeof(Module)),
                           &Address);
    Reply(Response, Request, Status, Address, 0, 0);
    return Status;
  }
  Reply(Response, Request, EFI_UNSUPPORTED, 0, 0, 0);
  return EFI_UNSUPPORTED;
}

static EFI_STATUS ProcessRequest(VOID) {
  REQUEST *Request;
  RESPONSE *Response;

  if (gMailboxPhysical == 0 || gMailboxSize < MAILBOX_SIZE) {
    return EFI_NOT_FOUND;
  }
  Request = (REQUEST *)(UINTN)gMailboxPhysical;
  Response = (RESPONSE *)(UINTN)(gMailboxPhysical + RESPONSE_OFFSET);
  return HandleRequest(Request, Response);
}

static EFI_STATUS EFIAPI SwSmiHandler(EFI_HANDLE DispatchHandle,
                                      const VOID *Context, VOID *CommBuffer,
                                      UINTN *CommBufferSize) {
  (void)DispatchHandle;
  (void)Context;
  (void)CommBuffer;
  (void)CommBufferSize;
  return ProcessRequest();
}

static EFI_STATUS RegisterSwSmi(VOID) {
  EFI_SMM_SW_DISPATCH2_PROTOCOL *SwDispatch = 0;
  EFI_SMM_SW_REGISTER_CONTEXT SwContext;
  EFI_LOCATE_PROTOCOL Locate;
  EFI_STATUS Status;

  if (gSwHandle != 0) {
    Log("smm sw smi already registered\n");
    return EFI_SUCCESS;
  }
  Locate = (EFI_LOCATE_PROTOCOL)gSmst->SmmLocateProtocol;
  Status = Locate(&gEfiSmmSwDispatch2ProtocolGuid, 0,
                  (VOID **)&SwDispatch);
  if (EFI_ERROR(Status) || SwDispatch == 0 || SwDispatch->Register == 0) {
    LogStatus("smm sw dispatch unavailable ",
              EFI_ERROR(Status) ? Status : EFI_NOT_FOUND);
    return EFI_ERROR(Status) ? Status : EFI_NOT_FOUND;
  }
  if ((UINTN)gSwSmiValue > SwDispatch->MaximumSwiValue) {
    Log("smm sw value out of range max=0x");
    LogHex(SwDispatch->MaximumSwiValue);
    Log(" requested=0x");
    LogHex(gSwSmiValue);
    Log("\n");
    return EFI_INVALID_PARAMETER;
  }
  SwContext.SwSmiInputValue = gSwSmiValue;
  Status = SwDispatch->Register(SwDispatch, SwSmiHandler, &SwContext,
                                &gSwHandle);
  if (EFI_ERROR(Status)) {
    LogStatus("smm sw smi register failed ", Status);
  } else {
    Log("smm sw smi registered value=0x");
    LogHex(gSwSmiValue);
    Log("\n");
  }
  return Status;
}

static EFI_STATUS ValidateSmst(VOID) {
  if (gSmst == 0) {
    Log("smm smst missing\n");
    return EFI_NOT_FOUND;
  }
  if (gSmst->SmmLocateProtocol == 0) {
    Log("smm locate protocol missing\n");
    return EFI_NOT_FOUND;
  }
  if (gSmst->SmmIo.Mem.Read == 0) {
    Log("smm mem read missing\n");
    return EFI_NOT_FOUND;
  }
  if (gSmst->SmmIo.Mem.Write == 0) {
    Log("smm mem write missing\n");
    return EFI_NOT_FOUND;
  }
  return EFI_SUCCESS;
}

static EFI_STATUS InitSmm(VOID) {
  EFI_SMM_BASE2_PROTOCOL *SmmBase = 0;
  EFI_STATUS Status;

  if (gSystemTable == 0 || gSystemTable->BootServices == 0) {
    Log("smm system table invalid\n");
    return EFI_INVALID_PARAMETER;
  }
  Status = gSystemTable->BootServices->LocateProtocol(
      &gEfiSmmBase2ProtocolGuid, 0, (VOID **)&SmmBase);
  if (EFI_ERROR(Status) || SmmBase == 0) {
    LogStatus("smm base2 unavailable ",
              EFI_ERROR(Status) ? Status : EFI_NOT_FOUND);
    return EFI_ERROR(Status) ? Status : EFI_NOT_FOUND;
  }
  Status = SmmBase->GetSmstLocation(SmmBase, &gSmst);
  if (EFI_ERROR(Status)) {
    LogStatus("smm get smst failed ", Status);
    return Status;
  }
  Status = ValidateSmst();
  if (EFI_ERROR(Status)) {
    return Status;
  }
  Log("smm smst ok cpus=0x");
  LogHex(gSmst->NumberOfCpus);
  Log("\n");
  return EFI_SUCCESS;
}

static EFI_STATUS ApplyConfig(CONFIG *Config, const char *Source) {
  EFI_STATUS Status;

  if (Config == 0 ||
      Config->Magic != CONFIG_MAGIC ||
      Config->MailboxPhysical == 0 ||
      Config->MailboxSize < MAILBOX_SIZE) {
    Log("smm config rejected from ");
    Log(Source);
    if (Config != 0) {
      Log(" magic=0x");
      LogHex(Config->Magic);
      Log(" mailbox=0x");
      LogHex(Config->MailboxPhysical);
      Log(" size=0x");
      LogHex(Config->MailboxSize);
    }
    Log("\n");
    return EFI_INVALID_PARAMETER;
  }
  gMailboxPhysical = Config->MailboxPhysical;
  gMailboxSize = Config->MailboxSize;
  if (Config->SwSmiValue != 0) {
    gSwSmiValue = Config->SwSmiValue;
  }
  Log("smm config from ");
  Log(Source);
  Log(" mailbox=0x");
  LogHex(gMailboxPhysical);
  Log(" size=0x");
  LogHex(gMailboxSize);
  Log(" sw=0x");
  LogHex(gSwSmiValue);
  Log("\n");
  Status = RegisterSwSmi();
  return Status;
}

static EFI_STATUS EFIAPI ConfigCommHandler(EFI_HANDLE DispatchHandle,
                                           const VOID *Context,
                                           VOID *CommBuffer,
                                           UINTN *CommBufferSize) {
  (void)DispatchHandle;
  (void)Context;
  (void)CommBufferSize;
  Log("smm config comm received\n");
  return ApplyConfig((CONFIG *)CommBuffer, "comm");
}

static EFI_STATUS RegisterConfigComm(VOID) {
  EFI_STATUS Status;

  if (gCommHandle != 0) {
    Log("smm config comm already registered\n");
    return EFI_SUCCESS;
  }
  if (gSmst->SmiHandlerRegister == 0) {
    Log("smm config comm register unavailable\n");
    return EFI_NOT_FOUND;
  }
  Status = gSmst->SmiHandlerRegister(ConfigCommHandler, &gConfigCommGuid,
                                      &gCommHandle);
  if (EFI_ERROR(Status)) {
    LogStatus("smm config comm register failed ", Status);
  } else {
    Log("smm config comm registered\n");
  }
  return Status;
}

static EFI_STATUS ConfigureFromPublishedTable(VOID) {
  CONFIG *Config = 0;
  UINTN Index;

  if (gSystemTable == 0 || gSystemTable->ConfigurationTable == 0) {
    Log("smm published config table unavailable\n");
    return EFI_NOT_FOUND;
  }
  for (Index = 0; Index < gSystemTable->NumberOfTableEntries; Index++) {
    EFI_CONFIGURATION_TABLE *Table = &gSystemTable->ConfigurationTable[Index];
    if (CompareGuid(&Table->VendorGuid, &gConfigGuid)) {
      Config = (CONFIG *)Table->VendorTable;
      break;
    }
  }
  if (Config == 0) {
    Log("smm published config missing\n");
    return EFI_NOT_FOUND;
  }
  return ApplyConfig(Config, "published");
}

EFI_STATUS EFIAPI SmmEntry(EFI_HANDLE ImageHandle,
                           EFI_SYSTEM_TABLE *SystemTable) {
  EFI_STATUS Status;
  (void)ImageHandle;

  gSystemTable = SystemTable;
  SerialInit();
  Log("mem smm init\n");
  Status = InitSmm();
  if (!EFI_ERROR(Status)) {
    Status = RegisterConfigComm();
    (void)Status;
    Status = ConfigureFromPublishedTable();
    if (EFI_ERROR(Status)) {
      LogStatus("smm published config failed ", Status);
    }
  } else {
    LogStatus("smm init failed ", Status);
  }
  Log("mem smm done\n");
  return EFI_SUCCESS;
}