// Injector.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <windows.h>
#include <Psapi.h>
#include <TlHelp32.h>
#include <string>
#include <vector>
#include <algorithm>

using namespace std;

int main()
{
    cout << "Forager DLL Injector" << endl;
    cout << "Trying to find game...";
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

DWORD findPidByName(const string& name)
{
    auto processes = listProcesses();
    string lowerName = name;
    
    // stack overflow (cursed) lambda for turning a string to lowercase
    transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return tolower(c); });

    for (auto& entry : processes)
    {
        string exe = entry.szExeFile;
         
    }
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
