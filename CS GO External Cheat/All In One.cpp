#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <thread>
#include <string>

DWORD csgoPID;
HANDLE csgoHandle;

HMODULE csgoHandleClient;
HMODULE csgoHandleEngine;

DWORD csgoLocalPlayer;

std::ptrdiff_t dwLocalPlayer;
std::ptrdiff_t dwEntityList;
std::ptrdiff_t dwForceAttack;
std::ptrdiff_t dwForceJump;
std::ptrdiff_t dwClientState;
std::ptrdiff_t dwClientState_ViewAngles;
std::ptrdiff_t dwGlowObjectManager;
std::ptrdiff_t dwPlayerResource;
std::ptrdiff_t dwGameRulesProxy;

std::ptrdiff_t m_bSpotted = 0x93D;
std::ptrdiff_t m_dwBoneMatrix = 0x26A8;
std::ptrdiff_t m_bDormant = 0xED;
std::ptrdiff_t m_vecOrigin = 0x138;
std::ptrdiff_t m_iGlowIndex = 0x10488;
std::ptrdiff_t m_iCrosshairId = 0x11838;
std::ptrdiff_t m_iTeamNum = 0xF4;
std::ptrdiff_t m_iHealth = 0x100;
std::ptrdiff_t m_hActiveWeapon = 0x2F08;
std::ptrdiff_t m_iItemDefinitionIndex = 0x2FBA;
std::ptrdiff_t m_bIsScoped = 0x9974;
std::ptrdiff_t m_fFlags = 0x104;
std::ptrdiff_t m_flFlashDuration = 0x10470;
std::ptrdiff_t m_aimPunchAngle = 0x303C;
std::ptrdiff_t m_bBombPlanted = 0x9A5;
std::ptrdiff_t m_iShotsFired = 0x103E0;

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

struct RGBA_s
{
	float r = 1.0f;
	float g = 0.0f;
	float b = 0.0f;
	float a = 1.0f;
};

struct renderWhen_s
{
	bool renderWhenOccluded = true;
	bool renderWhenUnOccluded = false;
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

MODULEENTRY32 GetModuleEntry(DWORD pid, const std::string& name)
{
	HANDLE snapshotHandle = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
	MODULEENTRY32 moduleEntry{};
	moduleEntry.dwSize = sizeof(moduleEntry);

	if (!Module32First(snapshotHandle, &moduleEntry))
	{
		CloseHandle(snapshotHandle);
		return MODULEENTRY32{};
	}

	do
	{
		if (std::equal(name.begin(), name.end(), moduleEntry.szModule))
		{
			CloseHandle(snapshotHandle);
			return moduleEntry;
		}
	} while (Module32Next(snapshotHandle, &moduleEntry));

	CloseHandle(snapshotHandle);
	return MODULEENTRY32{};
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

inline float Distance3D(Vector3 v31, Vector3 v32)
{
	return (float)sqrt(pow((v31.x - v32.x), 2) + pow((v31.y - v32.y), 2) + pow((v31.z - v32.z), 2));
}

uintptr_t FindPattern(HANDLE handle, MODULEENTRY32 moduleEntry, const char* pattern, uint32_t offset, uint32_t extra)
{
	uint8_t* bytes = new uint8_t[moduleEntry.modBaseSize];
	ReadProcessMemory(handle, moduleEntry.modBaseAddr, bytes, moduleEntry.modBaseSize, nullptr);

	uintptr_t scan = 0;
	const char* pat = pattern;
	uintptr_t firstMatch = 0;
	for (uintptr_t pCur = (uintptr_t)bytes; pCur < (uintptr_t)bytes + moduleEntry.modBaseSize; ++pCur) {
		if (!*pat) { scan = firstMatch; break; }
		if (*(uint8_t*)pat == '\?' || *(uint8_t*)pCur == ((((pat[0] & (~0x20)) >= 'A' && (pat[0] & (~0x20)) <= 'F') ? ((pat[0] & (~0x20)) - 'A' + 0xa) : ((pat[0] >= '0' && pat[0] <= '9') ? pat[0] - '0' : 0)) << 4 | (((pat[1] & (~0x20)) >= 'A' && (pat[1] & (~0x20)) <= 'F') ? ((pat[1] & (~0x20)) - 'A' + 0xa) : ((pat[1] >= '0' && pat[1] <= '9') ? pat[1] - '0' : 0)))) {
			if (!firstMatch) firstMatch = pCur;
			if (!pat[2]) { scan = firstMatch; break; }
			if (*(WORD*)pat == 16191 /*??*/ || *(uint8_t*)pat != '\?') pat += 3;
			else pat += 2;
		}
		else { pat = pattern; firstMatch = 0; }
	}
	if (!scan)
	{
		delete[] bytes;
		return 0x0;
	}
	uint32_t read = 0;
	ReadProcessMemory(handle, (void*)(scan - (uintptr_t)bytes + (uintptr_t)moduleEntry.modBaseAddr + offset), &read, sizeof(read), NULL);
	delete[] bytes;
	return read + extra - (uintptr_t)moduleEntry.modBaseAddr;
}

void RadarHack()
{
	while (true)
	{
		for (uint8_t i = 0; i < 64; i++)
		{
			WPM<bool>(csgoHandle, RPM<DWORD>(csgoHandle, (uintptr_t)csgoHandleClient + dwEntityList + i * 0x10) + m_bSpotted, true);
		}

		Sleep(1);
	}
}

void Triggerbot()
{
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
					int32_t csgoLocalPlayerWeaponID = RPM<int32_t>(csgoHandle, RPM<DWORD>(csgoHandle, (uintptr_t)csgoHandleClient + dwEntityList + ((csgoLocalPlayerWeapon & 0xFFF) - 1) * 0x10) + m_iItemDefinitionIndex);

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
}

void AutoJump()
{
	while (true)
	{
		if (GetAsyncKeyState(VK_SPACE) && RPM<uint8_t>(csgoHandle, csgoLocalPlayer + m_fFlags) & (1 << 0))
		{
			WPM<uint8_t>(csgoHandle, (uintptr_t)csgoHandleClient + dwForceJump, 6);
		}

		Sleep(1);
	}
}

void AntiFlashbang()
{
	while (true)
	{
		if (RPM<float>(csgoHandle, csgoLocalPlayer + m_flFlashDuration) > 0.0f)
		{
			WPM<float>(csgoHandle, csgoLocalPlayer + m_flFlashDuration, 0.0f);
		}

		Sleep(1);
	}
}

void NoRecoil()
{
	Vector3 lastAimPunchAngle;
	while (true)
	{
		DWORD csgoClientState = RPM<DWORD>(csgoHandle, (uintptr_t)csgoHandleEngine + dwClientState);

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
}

void GlowHack()
{
	while (true)
	{
		for (uint8_t i = 0; i < 64; i++)
		{
			DWORD entity = RPM<DWORD>(csgoHandle, (uintptr_t)csgoHandleClient + dwEntityList + i * 0x10);

			if (entity && RPM<int32_t>(csgoHandle, entity + m_iTeamNum) != RPM<int32_t>(csgoHandle, csgoLocalPlayer + m_iTeamNum))
			{
				int32_t glowIndex = RPM<int32_t>(csgoHandle, entity + m_iGlowIndex) * 0x38;
				DWORD glowObject = RPM<DWORD>(csgoHandle, (uintptr_t)csgoHandleClient + dwGlowObjectManager);

				WPM<RGBA_s>(csgoHandle, glowObject + glowIndex + 0x8, RGBA_s{});
				WPM<renderWhen_s>(csgoHandle, glowObject + glowIndex + 0x28, renderWhen_s{});
			}
		}

		Sleep(1);
	}
}

void RecoilCrosshair()
{
	HWND hwnd = FindWindow(nullptr, L"Counter-Strike: Global Offensive - Direct3D 9");

	HDC hdc = GetDC(hwnd);
	HBRUSH hbrush = CreateSolidBrush(RGB(255, 255, 255));

	RECT clientRect, crosshairRect;
	POINT crosshairPos, resolution;
	uint16_t crosshairSize = 5;

	while (true)
	{
		GetClientRect(hwnd, &clientRect);
		resolution = { clientRect.right - clientRect.left, clientRect.bottom - clientRect.top };

		Vector3 aimPunchAngle = RPM<Vector3>(csgoHandle, csgoLocalPlayer + m_aimPunchAngle);
		crosshairPos.x = (int32_t)(clientRect.left + (resolution.x / 2.0f) - (resolution.x / 90.0f * aimPunchAngle.y));
		crosshairPos.y = (int32_t)(clientRect.top + (resolution.y / 2.0f) - (resolution.y / 90.0f * -aimPunchAngle.x));

		crosshairRect = { crosshairPos.x - crosshairSize / 2, crosshairPos.y - crosshairSize / 2, crosshairPos.x + crosshairSize / 2, crosshairPos.y + crosshairSize / 2 };
		FillRect(hdc, &crosshairRect, hbrush);

		Sleep(1);
	}
}

void BombSiteLetter()
{
	int32_t lastC4Owner = 0;
	bool c4Once = true;

	while (true)
	{
		DWORD playerResource = RPM<DWORD>(csgoHandle, (uintptr_t)csgoHandleClient + dwPlayerResource);
		Vector3 bombsiteCenterA = RPM<Vector3>(csgoHandle, playerResource + 0x1664);
		Vector3 bombsiteCenterB = RPM<Vector3>(csgoHandle, playerResource + 0x1670);

		if (!(bombsiteCenterA.x == 0 && bombsiteCenterA.y == 0 && bombsiteCenterA.z == 0 && bombsiteCenterB.x == 0 && bombsiteCenterB.y == 0 && bombsiteCenterB.z == 0))
		{

			int32_t currentC4Owner = RPM<int32_t>(csgoHandle, playerResource + 0x165C);
			if (lastC4Owner != currentC4Owner)
			{
				lastC4Owner = currentC4Owner;
			}

			Vector3 c4OwnerPosition = RPM<Vector3>(csgoHandle, RPM<DWORD>(csgoHandle, (uintptr_t)csgoHandleClient + dwEntityList + (lastC4Owner - 1) * 0x10) + m_vecOrigin);

			if (RPM<bool>(csgoHandle, RPM<DWORD>(csgoHandle, (uintptr_t)csgoHandleClient + dwGameRulesProxy) + m_bBombPlanted) && c4Once)
			{
				c4Once = false;

				if (bombsiteCenterA.x == 0 && bombsiteCenterA.y == 0 && bombsiteCenterA.z == 0)
				{
					std::cout << 'B' << std::endl;
				}
				else if (bombsiteCenterB.x == 0 && bombsiteCenterB.y == 0 && bombsiteCenterB.z == 0)
				{
					std::cout << 'A' << std::endl;
				}
				else
				{
					std::cout << ((Distance3D(bombsiteCenterA, c4OwnerPosition) > Distance3D(bombsiteCenterB, c4OwnerPosition)) ? 'B' : 'A') << std::endl;
				}
			}

			if (!RPM<bool>(csgoHandle, RPM<DWORD>(csgoHandle, (uintptr_t)csgoHandleClient + dwGameRulesProxy) + m_bBombPlanted) && !c4Once)
			{
				c4Once = true;
			}
		}

		Sleep(1);
	}
}

void FastStop()
{
	while (true)
	{
		

		Sleep(1);
	}
}

void AimBot()
{
	while (true)
	{


		Sleep(1);
	}
}

int main()
{
	csgoPID = GetProcessID("csgo.exe");
	csgoHandle = OpenProcess(PROCESS_ALL_ACCESS, false, csgoPID);
	csgoHandleClient = GetModuleHandle(csgoPID, "client.dll");
	csgoHandleEngine = GetModuleHandle(csgoPID, "engine.dll");

	{
		MODULEENTRY32 csgoModuleEntryClient = GetModuleEntry(csgoPID, "client.dll");
		MODULEENTRY32 csgoModuleEntryEngine = GetModuleEntry(csgoPID, "engine.dll");

		dwLocalPlayer = FindPattern(csgoHandle, csgoModuleEntryClient, "8D 34 85 ? ? ? ? 89 15 ? ? ? ? 8B 41 08 8B 48 04 83 F9 FF", 0x3, 0x4);
		dwEntityList = FindPattern(csgoHandle, csgoModuleEntryClient, "BB ? ? ? ? 83 FF 01 0F 8C ? ? ? ? 3B F8", 0x1, 0x0);
		dwForceAttack = FindPattern(csgoHandle, csgoModuleEntryClient, "89 0D ? ? ? ? 8B 0D ? ? ? ? 8B F2 8B C1 83 CE 04", 0x2, 0x0);
		dwForceJump = FindPattern(csgoHandle, csgoModuleEntryClient, "8B 0D ? ? ? ? 8B D6 8B C1 83 CA 02", 0x2, 0x0);
		dwClientState = FindPattern(csgoHandle, csgoModuleEntryEngine, "A1 ? ? ? ? 33 D2 6A 00 6A 00 33 C9 89 B0", 0x1, 0x0);
		dwClientState_ViewAngles = 0x4D90 /*FindPattern(csgoHandle, csgoModuleEntryEngine, "F3 0F 11 86 ? ? ? ? F3 0F 10 44 24 ? F3 0F 11 86", 0x4, 0x0)*/;
		dwGlowObjectManager = FindPattern(csgoHandle, csgoModuleEntryClient, "A1 ? ? ? ? A8 01 75 4B", 0x1, 0x4);
		dwPlayerResource = FindPattern(csgoHandle, csgoModuleEntryClient, "8B 3D ? ? ? ? 85 FF 0F 84 ? ? ? ? 81 C7", 0x2, 0x0);
		dwGameRulesProxy = FindPattern(csgoHandle, csgoModuleEntryClient, "A1 ? ? ? ? 85 C0 0F 84 ? ? ? ? 80 B8 ? ? ? ? ? 74 7A", 0x1, 0x0);
	}

	std::thread radarHackThread([] { RadarHack(); });
	std::thread triggerbotThread([] { Triggerbot(); });
	std::thread autoJumpThread([] { AutoJump(); });
	std::thread antiFlashbangThread([] { AntiFlashbang(); });
	std::thread noRecoilThread([] { NoRecoil(); });
	std::thread glowHackThread([] { GlowHack(); });
	std::thread recoilCrosshairThread([] { RecoilCrosshair(); });
	std::thread bombSiteLetterThread([] { BombSiteLetter(); });
	std::thread aimBotThread([] { AimBot(); });
	std::thread fastStopThread([] { FastStop(); });

	radarHackThread.detach();
	triggerbotThread.detach();
	autoJumpThread.detach();
	antiFlashbangThread.detach();
	noRecoilThread.detach();
	glowHackThread.detach();
	recoilCrosshairThread.detach();
	bombSiteLetterThread.detach();
	aimBotThread.detach();
	fastStopThread.detach();

	while (true)
	{
		csgoLocalPlayer = RPM<DWORD>(csgoHandle, (uintptr_t)csgoHandleClient + dwLocalPlayer);

		Sleep(1);
	}

	return 0;
}
