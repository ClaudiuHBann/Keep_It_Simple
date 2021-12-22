#include <Windows.h>
#include <TlHelp32.h>
#include <string>

#define dwEntityList 0x4DCFA94
#define dwLocalPlayer 0xDB458C
#define dwForceAttack 0x31FFFA4
#define m_bIsScoped 0x9974
#define m_hActiveWeapon 0x2F08
#define m_iItemDefinitionIndex 0x2FBA
#define m_iTeamNum 0xF4
#define m_iCrosshairId 0x11838
#define m_iHealth 0x100

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
		if (GetAsyncKeyState(VK_MENU))
		{
			int32_t csgoLocalPlayerCrosshairID = RPM<int32_t>(csgoHandle, csgoLocalPlayer + m_iCrosshairId);

			if (csgoLocalPlayerCrosshairID != 0 && csgoLocalPlayerCrosshairID < 64)
			{
				DWORD entity = RPM<DWORD>(csgoHandle, (uintptr_t)csgoHandleClient + dwEntityList + ((csgoLocalPlayerCrosshairID - 1) * 0x10));

				if (RPM<int32_t>(csgoHandle, entity + m_iTeamNum) != RPM<int32_t>(csgoHandle, csgoLocalPlayer + m_iTeamNum) &&
					RPM<int32_t>(csgoHandle, entity + m_iHealth) > 0)
				{
					int32_t csgoLocalPlayerWeapon = RPM<int32_t>(csgoHandle, csgoLocalPlayer + m_hActiveWeapon);
					DWORD weaponEntity = RPM<DWORD>(csgoHandle, (uintptr_t)csgoHandleClient + dwEntityList + ((csgoLocalPlayerWeapon & 0xFFF) - 1) * 0x10);
					int32_t csgoLocalPlayerWeaponID = RPM<int32_t>(csgoHandle, weaponEntity + m_iItemDefinitionIndex);

					if (csgoLocalPlayerWeaponID == 40 || csgoLocalPlayerWeaponID == 9 || csgoLocalPlayerWeaponID == 11 || csgoLocalPlayerWeaponID == 38)
					{
						if (RPM<bool>(csgoHandle, csgoLocalPlayer + m_bIsScoped))
						{
							WPM<int32_t>(csgoHandle, (uintptr_t)csgoHandleClient + dwForceAttack, 5);
							Sleep(1);
							WPM<int32_t>(csgoHandle, (uintptr_t)csgoHandleClient + dwForceAttack, 4);
						}
					}
					else
					{
						WPM<int32_t>(csgoHandle, (uintptr_t)csgoHandleClient + dwForceAttack, 5);
						Sleep(1);
						WPM<int32_t>(csgoHandle, (uintptr_t)csgoHandleClient + dwForceAttack, 4);
					}
				}
			}
		}

		Sleep(1);
	}

	return 0;
}
