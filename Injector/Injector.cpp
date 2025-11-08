// Injector.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <windows.h>
#include <Psapi.h>
#include <TlHelp32.h>
#include <string>
#include <vector>
#include <algorithm>
#include <iomanip>

using namespace std;

#define DEBUG 0

const string TARGETEXE = "Forager.exe";
// currently an absolute path to my repository, later on I'll package it all into one zip.
const string MENUDLL = "C:\\Users\\User\\source\\repos\\Injector\\Debug\\CheatMenu.dll";

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
    cout << "Debug privileges accepted." << endl;
    cout << "Attempting to grab game handle..." << endl << endl;
    auto processes = listProcesses();

    // debug list processes during development
#if DEBUG
    cout << "PID\tExeName\t\tPath" << endl;
    cout << "------------------------------------------------------------" << endl;
    for (auto& proc : processes)
    {
        string path = getProcessImagePath(proc.th32ProcessID);
        cout << proc.th32ProcessID << "\t" << WCharToStr(proc.szExeFile) << "\t" << path << endl;
    }
#endif

    PROCESSENTRY32W foragerProcEntry{0};
    bool found = false;
    for (auto& proc : processes)
    {
        string currExe = WCharToStr(proc.szExeFile);
        if (currExe == TARGETEXE)
        {
            cout << "Found Forager process!" << endl;
            cout << "PID\tExeName\t\tPath" << endl;
            cout << "------------------------------------------------------------" << endl;
            cout << proc.th32ProcessID << "\t" << currExe << "\t" << getProcessImagePath(proc.th32ProcessID) << endl << endl;
            foragerProcEntry = proc;
            found = true;
        }
    }

    if (!found)
    {
        cout << "Could not locate Forager process, exiting." << endl;
        return -1;
    }

    // open proc handle for writing
    HANDLE hForager = OpenProcess(PROCESS_CREATE_THREAD |
        PROCESS_QUERY_INFORMATION |
        PROCESS_VM_OPERATION |
        PROCESS_VM_WRITE |
        PROCESS_VM_READ,
        FALSE,
        foragerProcEntry.th32ProcessID);

    // this is needed for null termination as std::string does not inherently include the terminating null byte
    auto strLen = MENUDLL.size() + 1;

    // allocate space for the menu DLL's path's size 
    auto memRegion = VirtualAllocEx(hForager, NULL, strLen, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    
    cout << "Found memregion at: " << memRegion << endl;

    // write the DLL path to that memory region
    WriteProcessMemory(hForager, memRegion, MENUDLL.c_str(), strLen, NULL);

    // grab LoadLibraryA's addr to call it to load the DLL
    auto loadLibraryAddr = GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
    
    // load the library in forager to 
    auto injectedThreadHandle = CreateRemoteThread(hForager, NULL, 0, (LPTHREAD_START_ROUTINE)loadLibraryAddr, memRegion, 0, NULL);

    if (!injectedThreadHandle)
    {
        cout << "Failed to inject DLL, exiting." << endl;
        return -1;
    }

    WaitForSingleObject(injectedThreadHandle, 5000);
    DWORD exitcode = 0;
    GetExitCodeThread(injectedThreadHandle, &exitcode);

    if (exitcode != 0)
    {
        cout << "Thread handle: 0x" << hex << injectedThreadHandle << endl;
        cout << "Allocated memory at: 0x" << hex << memRegion << " relative to the game." << endl;
        cout << "Successfully injected the DLL, unloading memory and exiting." << endl;
        VirtualFreeEx(hForager, memRegion, 0, MEM_RELEASE);
        CloseHandle(injectedThreadHandle);
        CloseHandle(hForager);
    }
    else
        cout << "Failed ot inject DLL, exit code: {" << exitcode << "}." << endl;

    return exitcode;
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
    return -1;
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