#define _WIN32_DCOM
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wbemidl.h>
#include <comdef.h>
#include <stdio.h>

#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

static void PrintError(const wchar_t *Operation, HRESULT Status) {
  fwprintf(stderr, L"%ls failed: 0x%08lX\n", Operation,
           (unsigned long)Status);
}

static void PrintCsvField(const wchar_t *Value) {
  const wchar_t *Cursor = Value == NULL ? L"" : Value;
  putwchar(L'"');
  while (*Cursor != L'\0') {
    if (*Cursor == L'"') {
      putwchar(L'"');
    }
    putwchar(*Cursor++);
  }
  putwchar(L'"');
}

int wmain(void) {
  HRESULT Status;
  IWbemLocator *Locator = NULL;
  IWbemServices *Services = NULL;
  IEnumWbemClassObject *Enumerator = NULL;
  BSTR Namespace = SysAllocString(L"ROOT\\WMI");
  BSTR QueryLanguage = SysAllocString(L"WQL");
  BSTR Query = SysAllocString(L"SELECT * FROM meta_class");
  ULONG Returned = 0;
  bool ComInitialized = false;
  int ExitCode = 1;

  Status = CoInitializeEx(NULL, COINIT_MULTITHREADED);
  if (FAILED(Status)) {
    PrintError(L"CoInitializeEx", Status);
    goto Cleanup;
  }
  ComInitialized = true;
  Status = CoInitializeSecurity(NULL, -1, NULL, NULL,
                                RPC_C_AUTHN_LEVEL_DEFAULT,
                                RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE,
                                NULL);
  if (FAILED(Status) && Status != RPC_E_TOO_LATE) {
    PrintError(L"CoInitializeSecurity", Status);
    goto Cleanup;
  }
  Status = CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER,
                            IID_IWbemLocator, (void **)&Locator);
  if (FAILED(Status)) {
    PrintError(L"CoCreateInstance(CLSID_WbemLocator)", Status);
    goto Cleanup;
  }
  Status = Locator->ConnectServer(Namespace, NULL, NULL, NULL, 0, NULL, NULL,
                                  &Services);
  if (FAILED(Status)) {
    PrintError(L"IWbemLocator::ConnectServer(ROOT\\WMI)", Status);
    goto Cleanup;
  }
  Status = CoSetProxyBlanket(Services, RPC_C_AUTHN_WINNT,
                             RPC_C_AUTHZ_NONE, NULL,
                             RPC_C_AUTHN_LEVEL_CALL,
                             RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
  if (FAILED(Status)) {
    PrintError(L"CoSetProxyBlanket", Status);
    goto Cleanup;
  }
  Status = Services->ExecQuery(QueryLanguage, Query,
                               WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                               NULL, &Enumerator);
  if (FAILED(Status)) {
    PrintError(L"IWbemServices::ExecQuery", Status);
    goto Cleanup;
  }

  wprintf(L"namespace,class_name\n");
  for (;;) {
    IWbemClassObject *Object = NULL;
    VARIANT Value;
    VariantInit(&Value);
    Status = Enumerator->Next(WBEM_INFINITE, 1, &Object, &Returned);
    if (Status == WBEM_S_FALSE || Returned == 0) {
      VariantClear(&Value);
      if (Object != NULL) {
        Object->Release();
      }
      break;
    }
    if (FAILED(Status)) {
      PrintError(L"IEnumWbemClassObject::Next", Status);
      VariantClear(&Value);
      if (Object != NULL) {
        Object->Release();
      }
      goto Cleanup;
    }
    Status = Object->Get(L"__CLASS", 0, &Value, NULL, NULL);
    if (SUCCEEDED(Status) && Value.vt == VT_BSTR) {
      PrintCsvField(Namespace);
      wprintf(L",");
      PrintCsvField(Value.bstrVal);
      wprintf(L"\n");
    }
    VariantClear(&Value);
    Object->Release();
  }
  ExitCode = 0;

Cleanup:
  if (Enumerator != NULL) {
    Enumerator->Release();
  }
  if (Services != NULL) {
    Services->Release();
  }
  if (Locator != NULL) {
    Locator->Release();
  }
  if (Namespace != NULL) {
    SysFreeString(Namespace);
  }
  if (QueryLanguage != NULL) {
    SysFreeString(QueryLanguage);
  }
  if (Query != NULL) {
    SysFreeString(Query);
  }
  if (ComInitialized) {
    CoUninitialize();
  }
  return ExitCode;
}
