#include "PayloadAbi.h"

EFI_STATUS EFIAPI PayloadEntry(PAYLOAD_CONTEXT *Context) {
  if (Context == 0 || Context->SerialPrint == 0 ||
      Context->SerialHex64 == 0) {
    return EFI_SUCCESS;
  }

  if (Context->Reason == REASON_UNLOAD) {
    Context->SerialPrint("payload unload\n");
    return EFI_SUCCESS;
  }
  if (Context->Reason == REASON_DOORBELL) {
    Context->SerialPrint("payload doorbell gen=0x");
    Context->SerialHex64(Context->Generation);
    Context->SerialPrint("\n");
    return EFI_SUCCESS;
  }

  Context->SerialPrint("payload load gen=0x");
  Context->SerialHex64(Context->Generation);
  Context->SerialPrint("\n");
  return EFI_SUCCESS;
}