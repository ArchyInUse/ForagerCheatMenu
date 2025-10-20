// Injector.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <windows.h>
#include <Psapi.h>
#include <TlHelp32.h>
#include <string>
#include <vector>
#include <algorithm>

#define DEBUG 1
using namespace std;

vector<PROCESSENTRY32> listProcesses();
DWORD findPidByName(const wstring& name);
string getProcessImagePath(DWORD pid);
string WCharToStr(WCHAR* wstr);
bool enableDebugPrivilege();

int main()
{
    cout << "Forager DLL Injector" << endl;
    cout << "Changing privilege token..." << endl;
    if (!enableDebugPrivilege())
    {
        string tmp;
        cout << "Debug privileges denied, press any button to exit the program." << endl;
        cin >> tmp;
        return 1;
    }
    cout << "Trying to find game..." << endl;
    auto processes = listProcesses();

#if DEBUG
    cout << "PID\tExeName\t\tPath" << endl;
    cout << "------------------------------------------------------------" << endl;
    for (auto& proc : processes)
    {
        string path = getProcessImagePath(proc.th32ProcessID);
        cout << proc.th32ProcessID << "\t" << WCharToStr(proc.szExeFile) << "\t" << path << endl;
    }
#endif 


    return 0;
}

vector<PROCESSENTRY32> listProcesses()
{
    vector<PROCESSENTRY32> list;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return list;

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(pe);
    if (Process32First(snapshot, &pe))
    {
        do {
            list.push_back(pe);
        } while (Process32Next(snapshot, &pe));
    }

    CloseHandle(snapshot);
    return list;
}

DWORD findPidByName(wstring& name)
{
    auto processes = listProcesses();
    wstring lowerName = name;
    
    // stack overflow (cursed) lambda for turning a string to lowercase
    transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return tolower(c); });

    for (auto& pe : processes) {
        wstring exe = pe.szExeFile;
        wstring lowerExe = exe;
        transform(lowerExe.begin(), lowerExe.end(), lowerExe.begin(), [](unsigned char c) { return tolower(c); });
        if (lowerExe == lowerName) {
            return pe.th32ProcessID;
        }
    }
}

string getProcessImagePath(DWORD pid)
{
    string path = "<access denied / system process>";
    HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, 0, pid);
    if (!hProc) return path;

    char buf[MAX_PATH] = { 0 };
    DWORD sz = MAX_PATH;

    if (QueryFullProcessImageNameA(hProc, NULL, buf, &sz))
    {
        path = string(buf);
    }
    else {
        if (GetModuleFileNameExA(hProc, 0, buf, MAX_PATH))
        {
            // fallback, most likely not going to be used but incase we have the PSAPI alternative
            path = string(buf);
        }
    }

    CloseHandle(hProc);
    return path;
}

string WCharToStr(WCHAR* wstr)
{
    int bytesNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
    string result;
    if (bytesNeeded > 0)
    {
        result.resize(bytesNeeded);
        WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &result[0], bytesNeeded, nullptr, nullptr);
        if (!result.empty() && result.back() == '\0') result.pop_back();
    } else {
        result.clear();
    }

    return result;
}

bool enableDebugPrivilege() {
    HANDLE hToken = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
        return false;

    TOKEN_PRIVILEGES tp;
    LUID luid;
    if (!LookupPrivilegeValue(nullptr, SE_DEBUG_NAME, &luid)) {
        CloseHandle(hToken);
        return false;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    BOOL ok = AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), nullptr, nullptr);
    CloseHandle(hToken);
    return ok && GetLastError() == ERROR_SUCCESS;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
