#include <iostream>
#include <Windows.h>
#include <string>
#include <WinInet.h>
#include <conio.h>
#include "sigscan.h"
#include <tchar.h>
#include <direct.h>
#include "VEH.h"
#include "xor.h"
#include <TlHelp32.h>

static const char rnum[] = "0123456789" "qwertyuiopasdfghjklzxcvbnm" "QWERTYUIOPASDFGHJKLZXCVBNM";
int staticLength = sizeof(rnum) - 1;
std::string RandomTitle;

void renamefile();

std::string getFilePath()
{
	char path[FILENAME_MAX];
	_getcwd(path, sizeof(path));
	return path;
}

void renamefile()
{
	TCHAR szExeFileName[MAX_PATH];
	GetModuleFileName(NULL, szExeFileName, MAX_PATH);
	std::string path = std::string(szExeFileName);
	std::string exe = path.substr(path.find_last_of("\\") + 1, path.size());

	srand(time(0));
	char letters[] = "01V8YutSgDmzEX8pK3gimydac1Sn2eWa9g3z";
	char newname[20];

	int z = rand() % 5 + 5;
	for (int i = 0; i < z; i++)
	{
		char x = letters[rand() % 36];
		newname[i] = x;
	}
	newname[z] = 0x0;
	strcat_s(newname, ".exe\0");
	rename(exe.c_str(), newname);
}

DWORD GetProcessByExeName(const wchar_t* ExeName)
{
	PROCESSENTRY32W pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32W);

	HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);
	if (hProcessSnap == INVALID_HANDLE_VALUE)
	{
		MessageBoxW(NULL, L"Error CreateToolhelp32Snapshot", L"Error!", NULL);
		return false;
	}

	if (Process32FirstW(hProcessSnap, &pe32))
	{
		do
		{
			if (_wcsicmp(pe32.szExeFile, ExeName) == 0)
			{
				CloseHandle(hProcessSnap);
				return pe32.th32ProcessID;
			}
		} while (Process32NextW(hProcessSnap, &pe32));
	}

	CloseHandle(hProcessSnap);
	return 0;
}

DWORD procID = GetProcessByExeName(L"javaw.exe");

char GetRandom()
{
	return rnum[rand() % staticLength];
}

void SetRandomTitle()
{
	srand(time(0));
	for (unsigned int i = 0; i < 20; ++i)
	{
		RandomTitle += GetRandom();
	}
	SetConsoleTitle(RandomTitle.c_str());
}

int GetJarPath(char* jar)
{
	std::cout << xorstr("Enter path to your plugin for loading (.jar): ") << std::endl;
	std::cin >> jar;
	return 1;
}

int GetProcess(HANDLE* hProc, DWORD pid)
{
	*hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procID);
	DWORD dwLastError = GetLastError();

	if (*hProc == NULL)
	{
		std::cout << xorstr("Unable to open javaw.exe process (guessing: memory is protected or javaw.exe is not running?)") << std::endl;
		return -1;
	}
	else
	{
		std::cout << xorstr("[LOG] javaw.exe -> OPENED") << std::endl;
		return 1;
	}
}

int Inject(DWORD pid, char* jar)
{
	HANDLE hProc;
	LPVOID TargetExplAddress;
	LPVOID BaseAddress;
	HANDLE RemoteThread;
	int JarLn;

	JarLn = strlen(jar) + 1;

	if (GetProcess(&hProc, procID) < 0)
		return -1;

	TargetExplAddress = (LPVOID)GetProcAddress(GetModuleHandleA("java.dll"), "Java_jdk_internal_loader_BootLoader_getSystemPackageLocation");
	if (!TargetExplAddress)
		return -1;
	
	BaseAddress = VirtualAllocEx(hProc, NULL, JarLn, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (!BaseAddress)
		return -1;
	if (!WriteProcessMemory(hProc, BaseAddress, jar, JarLn, NULL))
		return -1;

	RemoteThread = CreateRemoteThread(hProc, 0, 0, (LPTHREAD_START_ROUTINE)TargetExplAddress, BaseAddress, 0, 0);

	if (!RemoteThread)
		return -1;
	WaitForSingleObject(RemoteThread, INFINITE);
	VirtualFreeEx(hProc, BaseAddress, JarLn, MEM_RELEASE);

	if (CloseHandle(RemoteThread) == 0)
	{
		std::cout << xorstr("Error 165. Description: Failed to close handle to remote thread!") << std::endl;
		return -1;
	}
	if (CloseHandle(hProc) == 0)
	{
		std::cout << xorstr("Error 162. Description: Failed to close handle to javaw.exe!") << std::endl;
		return -1;
	}
}

int main()
{
	renamefile();
	SetRandomTitle();
	bool checkConn = InternetCheckConnectionA("https://www.google.com/", FLAG_ICC_FORCE_CONNECTION, 0);
	if (!checkConn)
	{
		std::cout << xorstr("You aren't connected to the internet!") << std::endl;
		Sleep(2000);
		exit(1);
	}
	char* jar = new char[255];

	GetJarPath(jar);
	Inject(procID, jar);
	std::cout << xorstr("[LOG] javaw.exe - ") << jar << " - INJECTED!" << std::endl;
	system("pause");
	return 0;
}
