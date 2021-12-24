#include <Windows.h>
#include <TlHelp32.h>
#include <string>

#define dwLocalPlayer 0xDB458C
#define m_aimPunchAngle 0x303C
#define dwClientState 0x589FC4
#define m_iShotsFired 0x103E0
#define dwClientState_ViewAngles 0x4D90

class Vector3
{
public:
	float x, y, z;

	Vector3(float x = 0.0f, float y = 0.0f, float z = 0.0f)
	{
		this->x = x;
		this->y = y;
		this->z = z;
	}

	void Normalize()
	{
		while (y < -180)
		{
			y += 360;
		}

		while (y > 180)
		{
			y -= 360;
		}

		if (x > 89)
		{
			x = 89;
		}

		if (x < -89)
		{
			x = -89;
		}
	}

	Vector3 operator+(Vector3 vector3)
	{
		return { x + vector3.x, y + vector3.y, z + vector3.z };
	}

	Vector3 operator-(Vector3 vector3)
	{
		return { x - vector3.x, y - vector3.y, z - vector3.z };
	}

	Vector3 operator*(float f)
	{
		return { x * f, y * f, z * f };
	}
};

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
	HMODULE csgoHandleEngine = GetModuleHandle(csgoPID, "engine.dll");

	DWORD csgoLocalPlayer = RPM<DWORD>(csgoHandle, (uintptr_t)csgoHandleClient + dwLocalPlayer);
	DWORD csgoClientState = RPM<DWORD>(csgoHandle, (uintptr_t)csgoHandleEngine + dwClientState);

	Vector3 lastAimPunchAngle;
	while (true)
	{
		Vector3 viewAngles = RPM<Vector3>(csgoHandle, csgoClientState + dwClientState_ViewAngles);
		Vector3 aimPunchAngle = RPM<Vector3>(csgoHandle, csgoLocalPlayer + m_aimPunchAngle) * 2;

		if (RPM<int32_t>(csgoHandle, csgoLocalPlayer + m_iShotsFired) > 1)
		{
			Vector3 newAngle = viewAngles + lastAimPunchAngle - aimPunchAngle;
			newAngle.Normalize();

			WPM<Vector3>(csgoHandle, csgoClientState + dwClientState_ViewAngles, newAngle);
		}

		lastAimPunchAngle = aimPunchAngle;

		Sleep(1);
	}

	return 0;
}
