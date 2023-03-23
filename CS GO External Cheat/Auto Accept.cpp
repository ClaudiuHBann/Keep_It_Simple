#include <Windows.h>
//
#include <TlHelp32.h>

#include <string>

using namespace std;

#define BUTTON_RATIO_POS_X (0.444f)
#define BUTTON_RATIO_POS_Y (0.377f)

#define BUTTON_RATIO_SIZE_W (0.117f)
#define BUTTON_RATIO_SIZE_H (0.078f)

#define BUTTON_COLOR (5287756L)
#define BUTTON_COLOR_ERROR (12ui8)

class AutoAcceptCSGO
{
  public:
    inline AutoAcceptCSGO() : mHWND(FindHWND(GetPID(L"csgo.exe"))), mHDC(GetDC(mHWND))
    {
    }

    inline ~AutoAcceptCSGO()
    {
        if (mHWND && mHDC)
        {
            ReleaseDC(mHWND, mHDC);
        }
    }

    bool Check()
    {
        RECT rect;
        GetClientRectForce(rect);

        LONG posX = (LONG)((float)rect.right * BUTTON_RATIO_POS_X + (float)rect.right * BUTTON_RATIO_SIZE_W / 2.f);
        LONG posYBegin = (LONG)((float)rect.bottom * BUTTON_RATIO_POS_Y);
        LONG posYEnd = (LONG)((float)rect.bottom * BUTTON_RATIO_POS_Y + (float)rect.bottom * BUTTON_RATIO_SIZE_H);

        for (auto posY = posYBegin; posY <= posYEnd; posY++)
        {
            auto colorrefCurrent = GetPixel(mHDC, posX, posY);
            RGBTRIPLE rgbtripleCurrent{GetRValue(colorrefCurrent), GetGValue(colorrefCurrent),
                                       GetBValue(colorrefCurrent)};

            COLORREF colorrefFinal = BUTTON_COLOR;
            RGBTRIPLE rgbtripleFinal{GetRValue(colorrefFinal), GetGValue(colorrefFinal), GetBValue(colorrefFinal)};

            if (ValueInValueWithError(rgbtripleCurrent.rgbtRed, rgbtripleFinal.rgbtRed, BUTTON_COLOR_ERROR) &&
                ValueInValueWithError(rgbtripleCurrent.rgbtGreen, rgbtripleFinal.rgbtGreen, BUTTON_COLOR_ERROR) &&
                ValueInValueWithError(rgbtripleCurrent.rgbtBlue, rgbtripleFinal.rgbtBlue, BUTTON_COLOR_ERROR))
            {
                return true;
            }
        }

        return false;
    }

    inline bool Accept()
    {
        RECT rect;
        GetClientRectForce(rect);

        POINT point{
            (LONG)((float)rect.right * BUTTON_RATIO_POS_X) + (LONG)((float)rect.right * BUTTON_RATIO_SIZE_W / 2.f),
            (LONG)((float)rect.bottom * BUTTON_RATIO_POS_Y) + (LONG)((float)rect.bottom * BUTTON_RATIO_SIZE_H / 2.f)};

        return !SendMessage(mHWND, WM_MOUSEMOVE, 0, MAKELPARAM(point.x, point.y)) &&
               !SendMessage(mHWND, WM_LBUTTONDOWN, MK_LBUTTON, 0) &&
               !SendMessage(mHWND, WM_MOUSEMOVE, 0, MAKELPARAM(point.x, point.y)) &&
               !SendMessage(mHWND, WM_LBUTTONUP, MK_LBUTTON, 0);
    }

  private:
    static DWORD GetPID(const wstring &processName)
    {
        auto snapshotHandle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        PROCESSENTRY32 processEntry{.dwSize = sizeof(processEntry)};

        if (!Process32First(snapshotHandle, &processEntry))
        {
            CloseHandle(snapshotHandle);

            return 0;
        }

        do
        {
            if (processName == processEntry.szExeFile)
            {
                CloseHandle(snapshotHandle);

                return processEntry.th32ProcessID;
            }
        } while (Process32Next(snapshotHandle, &processEntry));

        CloseHandle(snapshotHandle);

        return 0;
    }

    typedef struct
    {
        DWORD pid;
        HWND hwnd;
    } findHWNDData;

    static inline HWND FindHWND(const DWORD pid)
    {
        findHWNDData data{.pid = pid};

        EnumWindows(EnumWindowsCallback, (LPARAM)&data);

        return data.hwnd;
    }

    static inline BOOL CALLBACK EnumWindowsCallback(HWND hwnd, LPARAM lParam)
    {
        auto &data = *reinterpret_cast<findHWNDData *>(lParam);

        DWORD pid;
        GetWindowThreadProcessId(hwnd, &pid);
        if (data.pid != pid || !IsMainWindow(hwnd))
        {
            return TRUE;
        }

        data.hwnd = hwnd;

        return FALSE;
    }

    static inline bool IsMainWindow(HWND hwnd)
    {
        return !GetWindow(hwnd, GW_OWNER) && IsWindowVisible(hwnd);
    }

    inline bool ShowWithNoActivateIfNeeded()
    {
        if (IsIconic(mHWND))
        {
            ShowWindow(mHWND, SW_SHOWNOACTIVATE);
        }

        return true;
    }

    inline bool GetClientRectForce(RECT &rect)
    {
        return ShowWithNoActivateIfNeeded() && GetClientRect(mHWND, &rect);
    }

    template <typename T>
    static inline bool ValueInValueWithError(const T &valueToCheck, const T &valueGood, const T &error = {})
    {
        return valueGood - error <= valueToCheck && valueToCheck <= valueGood + error;
    }

    HWND mHWND{};
    HDC mHDC{};
};

int main()
{
    AutoAcceptCSGO aaCSGO;

    while (true)
    {
        Sleep(1000);

        if (aaCSGO.Check())
        {
            aaCSGO.Accept();
        }
    }

    return 0;
}
