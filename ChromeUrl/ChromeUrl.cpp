#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <comutil.h>
#include <AtlBase.h>
#include <UIAutomation.h>
#pragma comment(lib,"comsuppw.lib")

std::string CurrentUrl;
HWND ChromeWidgetHwnd = NULL;

DWORD WINAPI GetChromeUrl(LPVOID lpParameter) {
    int RetInitializeEx = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    while (true) {
        if (!IsWindowVisible(ChromeWidgetHwnd) && GetWindowTextLength(ChromeWidgetHwnd) == 0) {
            break;
        }
        CComPtr<IUIAutomation> Automation;
        if SUCCEEDED(Automation.CoCreateInstance(CLSID_CUIAutomation)) {
            CComPtr<IUIAutomationElement> Root;
            if (SUCCEEDED(Automation->ElementFromHandle(ChromeWidgetHwnd, &Root))) {
                CComPtr<IUIAutomationElement> Pane;
                CComPtr<IUIAutomationCondition> PaneCondition;
                Automation->CreatePropertyCondition(UIA_ControlTypePropertyId, CComVariant(UIA_PaneControlTypeId), &PaneCondition);
                CComPtr<IUIAutomationElementArray> ElementArray;
                if (FAILED(Root->FindAll(TreeScope_Children, PaneCondition, &ElementArray))) {
                    break;
                }
                int Count = 0;
                ElementArray->get_Length(&Count);
                for (int i = 0; i < Count; i++) {
                    CComBSTR name;
                    if (SUCCEEDED(ElementArray->GetElement(i, &Pane))) {
                        if (SUCCEEDED(Pane->get_CurrentName(&name))) {
                            if (wcscmp(name, L"Google Chrome") == 0) {
                                break;
                            }
                        }
                    }
                    Pane.Release();
                }
                if (!Pane) {
                    break;
                }
                CComPtr<IUIAutomationElement> Url;
                CComPtr<IUIAutomationCondition> UrlCondition;
                Automation->CreatePropertyCondition(UIA_ControlTypePropertyId, CComVariant(UIA_EditControlTypeId), &UrlCondition);
                if (FAILED(Pane->FindFirst(TreeScope_Descendants, UrlCondition, &Url))) {
                    break;
                }
                CComVariant Variant;
                if (FAILED(Url->GetCurrentPropertyValue(UIA_ValueValuePropertyId, &Variant))) {
                    break;
                }
                if (!Variant.bstrVal) {
                    break;
                }
                _bstr_t ConvertString = _bstr_t(Variant.bstrVal);
                char* Data = new char[ConvertString.length()];
                strcpy_s(Data, ConvertString.length() + 1, ConvertString);
                CurrentUrl = Data;
            }  
            Automation.Release();
        }
        CoUninitialize();
    }
    return 0;
}

void MainLoop() {
    std::cout << CurrentUrl << std::endl;
}

int main() {
    while (true) {
        DWORD ProcessID = 0;
        PROCESSENTRY32 ProcessEntry;
        HANDLE Snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        ProcessEntry.dwSize = sizeof(PROCESSENTRY32);
        if (Process32First(Snapshot, &ProcessEntry)) {
            do {
                if (!lstrcmpi(ProcessEntry.szExeFile, "chrome.exe")) {
                    CloseHandle(Snapshot);
                    ProcessID = ProcessEntry.th32ProcessID;
                }
            } while (Process32Next(Snapshot, &ProcessEntry));
        }
        CloseHandle(Snapshot);
        Sleep(100);
        if (ProcessID > 0) {
            ChromeWidgetHwnd = FindWindowEx(NULL, ChromeWidgetHwnd, "Chrome_WidgetWin_1", NULL);
            if ((ChromeWidgetHwnd != NULL) && (IsWindowVisible(ChromeWidgetHwnd) && GetWindowTextLength(ChromeWidgetHwnd) > 0)) {
                break;
            }
        }
    }

    HANDLE hThread = CreateThread(NULL, 0, GetChromeUrl, NULL, 0, NULL);
    if (hThread == NULL) {
        return 1;
    }

    while (true) {
        MainLoop();
    }

    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
    return 0;
}