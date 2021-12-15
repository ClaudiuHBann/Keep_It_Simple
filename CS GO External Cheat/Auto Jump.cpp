#include <Windows.h>
#include <TlHelp32.h>
#include <string>

#define dwLocalPlayer 0xDB458C
#define m_fFlags 0x104
#define dwForceJump 0x527998C

template<typename T>
inline T RPM(HANDLE handle, const uintptr_t address)
{
	T t;
	ReadProcessMemory(handle, (LPCVOID)address, &t, sizeof(t), nullptr);

	return t;
}

template<typename T>
inline BOOL WPM(HANDLE handle, const uintptr_t address, const T& t)
{
	return WriteProcessMemory(handle, (LPVOID)address, &t, sizeof(t), nullptr);
}

HMODULE GetModuleHandle(DWORD pid, const std::string& name)
{
	HANDLE snapshotHandle = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
	MODULEENTRY32 moduleEntry;
	moduleEntry.dwSize = sizeof(moduleEntry);

	if (!Module32First(snapshotHandle, &moduleEntry))
	{
		CloseHandle(snapshotHandle);
		return 0;
	}

	do
	{
		if (std::equal(name.begin(), name.end(), moduleEntry.szModule))
		{
			CloseHandle(snapshotHandle);
			return moduleEntry.hModule;
		}
	} while (Module32Next(snapshotHandle, &moduleEntry));

	CloseHandle(snapshotHandle);
	return 0;
}

DWORD GetProcessID(const std::string& name)
{
	HANDLE snapshotHandle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32 processEntry;
	processEntry.dwSize = sizeof(processEntry);

	if (!Process32First(snapshotHandle, &processEntry))
	{
		CloseHandle(snapshotHandle);
		return 0;
	}

	do
	{
		if (std::equal(name.begin(), name.end(), processEntry.szExeFile))
		{
			CloseHandle(snapshotHandle);
			return processEntry.th32ProcessID;
		}
	} while (Process32Next(snapshotHandle, &processEntry));

	CloseHandle(snapshotHandle);
	return 0;
}

int main()
{
	DWORD csgoPID = GetProcessID("csgo.exe");
	HANDLE csgoHandle = OpenProcess(PROCESS_ALL_ACCESS, false, csgoPID);

	HMODULE csgoHandleClient = GetModuleHandle(csgoPID, "client.dll");
	DWORD csgoLocalPlayer = RPM<DWORD>(csgoHandle, (uintptr_t)csgoHandleClient + dwLocalPlayer);

	while (true)
	{
		uint8_t flagsResult = RPM<uint8_t>(csgoHandle, csgoLocalPlayer + m_fFlags);

		if (GetAsyncKeyState(VK_SPACE) && flagsResult & (1 << 0))
		{
			WPM<uint8_t>(csgoHandle, (uintptr_t)csgoHandleClient + dwForceJump, 6);
		}

		Sleep(1);
	}


	return 0;
}
