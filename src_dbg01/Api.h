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
#ifndef API_H
#define API_H

#include <stdint.h>

#define NAME_SIZE 64U

typedef struct {
  uint32_t Pid;
  uint32_t Reserved;
  uint64_t Eprocess;
  uint64_t Cr3;
  uint64_t ImageBase;
  char Name[NAME_SIZE];
} PROCESS_INFO;

typedef struct {
  uint32_t Pid;
  uint32_t Reserved;
  uint64_t Base;
  uint64_t Size;
  uint64_t Cr3;
  char Name[NAME_SIZE];
} MODULE_INFO;

typedef int (*DUMP_CALLBACK)(uint64_t Address, const void *Data,
                             uint32_t Size, void *Context);

int Init(void);
void Close(void);
int Ping(void);
int FindProcessByPid(uint32_t Pid, PROCESS_INFO *Process);
int FindProcessByName(const char *Name, PROCESS_INFO *Process);
int TranslateVirt(uint32_t Pid, uint64_t Va, uint64_t *Pa);
int ReadPhys(uint64_t Address, void *Buffer, uint32_t Size);
int WritePhys(uint64_t Address, const void *Buffer, uint32_t Size);
int ReadVirt(uint32_t Pid, uint64_t Address, void *Buffer, uint32_t Size);
int WriteVirt(uint32_t Pid, uint64_t Address, const void *Buffer,
              uint32_t Size);
int FindModule(const PROCESS_INFO *Process, const char *Name,
               MODULE_INFO *Module);
int FindKernelModule(const char *Name, MODULE_INFO *Module);
int FindExport(const MODULE_INFO *Module, const char *Name,
               uint64_t *Address);
int Dump(const MODULE_INFO *Module, DUMP_CALLBACK Callback, void *Context);

#endif
