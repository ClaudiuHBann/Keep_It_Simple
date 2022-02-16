#include <Windows.h>
#include <TlHelp32.h>
#include <string>

#define dwLocalPlayer 0xDB35EC
#define m_aimPunchAngle 0x303C

class Vector3
{
public:
	float x, y, z;
};

template<typename T>
inline T RPM(HANDLE handle, const uintptr_t address)
{
	T t = T();
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
	MODULEENTRY32 moduleEntry{};
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
	PROCESSENTRY32 processEntry{};
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

	HWND hwnd = FindWindow(nullptr, L"Counter-Strike: Global Offensive - Direct3D 9");
	HDC hdc = GetDC(hwnd);
	HBRUSH hbrush = CreateSolidBrush(RGB(255, 0, 0));

	RECT clientRect, crosshairRect;
	POINT crosshairPos, resolution;
	uint16_t crosshairSize = 5;

	while (true)
	{
		DWORD csgoLocalPlayer = RPM<DWORD>(csgoHandle, (uintptr_t)csgoHandleClient + dwLocalPlayer);
		GetClientRect(hwnd, &clientRect);
		resolution = { clientRect.right - clientRect.left, clientRect.bottom - clientRect.top };

		Vector3 aimPunchAngle = RPM<Vector3>(csgoHandle, csgoLocalPlayer + m_aimPunchAngle);
		crosshairPos.x = (int32_t)(clientRect.left + (resolution.x / 2.0f) - (resolution.x / 90.0f * aimPunchAngle.y));
		crosshairPos.y = (int32_t)(clientRect.top + (resolution.y / 2.0f) - (resolution.y / 90.0f * -aimPunchAngle.x));

		crosshairRect = { crosshairPos.x - crosshairSize / 2, crosshairPos.y - crosshairSize / 2, crosshairPos.x + crosshairSize / 2, crosshairPos.y + crosshairSize / 2 };
		FillRect(hdc, &crosshairRect, hbrush);

		Sleep(1);
	}

	return 0;
}
