/**
 * MIT License
 * 
 * Copyright (c) 2026 Rayan Ziani
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
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
