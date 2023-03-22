/*
    Code taken from project https://github.com/ClaudiuHBann/Web_Scraper_v2
*/

#define _UNICODE
#define UNICODE

#include <Urlmon.h>
#pragma comment(lib, "urlmon.lib")

#include <wininet.h>
#pragma comment(lib, "Wininet.lib")

#include <filesystem>
#include <functional>
#include <iostream>
#include <sstream>
#include <cassert>

using namespace std;
using namespace filesystem;

#include <io.h>
#include <fcntl.h>

constexpr auto DEFAULT_CONTEXT = L"HBann";

class InternetStatus
{
  public:
    InternetStatus(
        const function<void(HINTERNET, DWORD_PTR, DWORD, LPVOID, DWORD)> &callback =
            +[](HINTERNET hInternet, DWORD_PTR dwContext, DWORD dwInternetStatus, LPVOID lpvStatusInformation,
                DWORD dwStatusInformationLength) {
                wcout << L"HINTERNET: " << (wstringstream() << hInternet).str() << endl;
                wcout << L"Context: " << dwContext << endl;
                wcout << L"Status code: " << dwInternetStatus << endl;

                if (lpvStatusInformation)
                {
                    wcout << L"Status info code: " << *(DWORD *)lpvStatusInformation << endl;
                    wcout << L"Status info size: " << dwStatusInformationLength << endl;
                }

                wcout << endl;
            },
        const DWORD_PTR context = *(DWORD_PTR *)DEFAULT_CONTEXT)
        : mCallback(callback), mContext(context)
    {
    }

    bool SetInstance(HINTERNET hInternet) const
    {
        if (!hInternet)
        {
            return false;
        }

        auto target = mCallback.target<void (*)(HINTERNET, DWORD_PTR, DWORD, LPVOID, DWORD)>();
        assert(target && "Decay your lambda like +[](){} !");

        return InternetSetStatusCallback(hInternet, *target) != INTERNET_INVALID_STATUS_CALLBACK;
    }

    void ResetInstance(HINTERNET hInternet) const
    {
        if (!hInternet)
        {
            return;
        }

        InternetSetStatusCallback(hInternet, nullptr);
    }

    function<void(HINTERNET, DWORD_PTR, DWORD, LPVOID, DWORD)> GetCallback() const
    {
        return mCallback;
    }

    void SetCallback(function<void(HINTERNET, DWORD_PTR, DWORD, LPVOID, DWORD)> callback)
    {
        mCallback = callback;
    }

    DWORD_PTR GetContext() const
    {
        return mContext;
    }

    void SetContext(const DWORD_PTR context)
    {
        mContext = context;
    }

  private:
    function<void(HINTERNET, DWORD_PTR, DWORD, LPVOID, DWORD)> mCallback{};
    DWORD_PTR mContext{};
};

class BindStatus : public IBindStatusCallback
{
  public:
    function<HRESULT(ULONG, ULONG, ULONG, LPCWSTR)> CallbackOnProgress = [](ULONG ulProgress, ULONG ulProgressMax,
                                                                            ULONG ulStatusCode, LPCWSTR wszStatusText) {
        wcout << L"OnProgess: Downloaded " << ulProgress << L" of " << ulProgressMax << L" byte(s). Status: "
              << (wszStatusText ? wszStatusText : L"-") << L" (" << ulStatusCode << L")" << endl;
        return S_OK;
    };

    function<HRESULT(HRESULT, LPCWSTR)> CallbackOnStopBinding = [](HRESULT hresult, LPCWSTR szError) {
        wcout << L"OnStopBinding: Status: " << (szError ? szError : L"-") << L" (" << hresult << L")" << endl;
        return S_OK;
    };

    function<HRESULT(IBinding *)> CallbackOnStartBinding = [](auto *) { return S_OK; };
    function<HRESULT(LONG *)> CallbackGetPriority = [](auto *) { return S_OK; };
    function<HRESULT(DWORD *, BINDINFO *)> CallbackGetBindInfo = [](auto *, auto *) { return S_OK; };
    function<HRESULT(DWORD, DWORD, FORMATETC *, STGMEDIUM *)> CallbackOnDataAvailable = [](auto, auto, auto *, auto *) {
        return S_OK;
    };
    function<HRESULT(REFIID, IUnknown *)> CallbackOnObjectAvailable = [](const auto &, auto *) { return S_OK; };

  private:
    HRESULT STDMETHODCALLTYPE OnStartBinding(DWORD /*dwReserved*/, __RPC__in_opt IBinding *pib) override
    {
        return CallbackOnStartBinding(pib);
    }

    HRESULT STDMETHODCALLTYPE GetPriority(__RPC__out LONG *pnPriority) override
    {
        return CallbackGetPriority(pnPriority);
    }

    HRESULT STDMETHODCALLTYPE OnLowResource(DWORD reserved) override
    {
        wcout << L"BindStatus::OnLowResource(" << reserved << ")" << endl;
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode,
                                         __RPC__in_opt LPCWSTR wszStatusText) override
    {
        return CallbackOnProgress(ulProgress, ulProgressMax, ulStatusCode, wszStatusText);
    }

    HRESULT STDMETHODCALLTYPE OnStopBinding(HRESULT hresult, __RPC__in_opt LPCWSTR szError) override
    {
        return CallbackOnStopBinding(hresult, szError);
    }

    HRESULT STDMETHODCALLTYPE GetBindInfo(DWORD *grfBINDF, BINDINFO *pbindinfo) override
    {
        return CallbackGetBindInfo(grfBINDF, pbindinfo);
    }

    HRESULT STDMETHODCALLTYPE OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC *pformatetc,
                                              STGMEDIUM *pstgmed) override
    {
        return CallbackOnDataAvailable(grfBSCF, dwSize, pformatetc, pstgmed);
    }

    HRESULT STDMETHODCALLTYPE OnObjectAvailable(__RPC__in REFIID riid, __RPC__in_opt IUnknown *punk) override
    {
        return CallbackOnObjectAvailable(riid, punk);
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID /*riid*/,
                                             _COM_Outptr_ void __RPC_FAR *__RPC_FAR * /*ppvObject*/) override
    {
        return S_OK;
    }

    ULONG STDMETHODCALLTYPE AddRef(void) override
    {
        return 0;
    }

    ULONG STDMETHODCALLTYPE Release(void) override
    {
        return 0;
    }
};

constexpr auto DEFAULT_USER_AGENT =
    L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/109.0.0.0 Safari/537.36";
constexpr auto DEFAULT_BUFFER_SIZE = 8192;

class WebScraper
{
  public:
    WebScraper(const wstring &userAgent = DEFAULT_USER_AGENT, const DWORD accessType = INTERNET_OPEN_TYPE_PRECONFIG,
               const wstring &proxy = L"", const wstring &proxyBypass = L"", const DWORD flagsOpen = 0)
    {
        mHInternetOpen = InternetOpen(userAgent.c_str(), accessType, proxy.empty() ? nullptr : proxy.c_str(),
                                      proxyBypass.empty() ? nullptr : proxyBypass.c_str(), flagsOpen);
    }

    ~WebScraper()
    {
        if (mHInternetOpen)
        {
            InternetCloseHandle(mHInternetOpen);
        }
    }

    void SetBufferSize(const DWORD bufferSize)
    {
        mBufferSize = bufferSize;
    }

    DWORD GetBufferSize() const
    {
        return mBufferSize;
    }

    static bool URLToFile(const wstring &url, const wstring &file, BindStatus *bindStatus = nullptr)
    {
        path filePath(file.c_str());
        create_directories(filePath.parent_path());

        return SUCCEEDED(::URLDownloadToFile(nullptr, url.c_str(), file.c_str(), 0, bindStatus));
    }

    static bool URLToFileCache(const wstring &url, wstring &file, BindStatus *bindStatus = nullptr)
    {
        wchar_t buffer[MAX_PATH];
        auto result = ::URLDownloadToCacheFile(nullptr, url.c_str(), buffer, MAX_PATH, 0, bindStatus);

        file.assign(buffer);
        return SUCCEEDED(result);
    }

    bool URLToString(const wstring &url, wstring &str, const InternetStatus *internetStatusOpenURL = nullptr,
                     const wstring &header = L"", const DWORD flagsOpenURL = 0, const DWORD flagsReadFileEx = 0,
                     const DWORD_PTR contextReadFileEx = 0)
    {
        if (!mHInternetOpen)
        {
            return false;
        }

        auto hInternetOpenUrl = ::InternetOpenUrl(
            mHInternetOpen, url.c_str(), header.empty() ? nullptr : header.c_str(), header.empty() ? 0 : -1L,
            flagsOpenURL, internetStatusOpenURL ? internetStatusOpenURL->GetContext() : 0);
        if (!hInternetOpenUrl)
        {
            return false;
        }

        if (internetStatusOpenURL && !internetStatusOpenURL->SetInstance(hInternetOpenUrl))
        {
            return false;
        }

        auto buffer = new char[mBufferSize];

        INTERNET_BUFFERS bufferInternet{.dwStructSize = sizeof(INTERNET_BUFFERS), .lpvBuffer = buffer};

        BOOL result;
        string strBase;
        do
        {
            bufferInternet.dwBufferLength = mBufferSize;

            result = ::InternetReadFileEx(hInternetOpenUrl, &bufferInternet, flagsReadFileEx, contextReadFileEx);
            if (!result || !bufferInternet.dwBufferLength)
            {
                break;
            }

            strBase.append(buffer, bufferInternet.dwBufferLength);
        } while (true);
        str = wstring(strBase.begin(), strBase.end());

        delete[] buffer;
        if (internetStatusOpenURL)
        {
            internetStatusOpenURL->ResetInstance(hInternetOpenUrl);
        }
        ::InternetCloseHandle(hInternetOpenUrl);

        return result;
    }

  private:
    HINTERNET mHInternetOpen{};
    DWORD mBufferSize = DEFAULT_BUFFER_SIZE;
};

int main()
{
    auto &&_ = _setmode(_fileno(stdout), _O_U16TEXT);
    _;

    WebScraper scraper;
    wstring url(L"https://github.com/ClaudiuHBann?tab=repositories");

    wstring wstr;
    InternetStatus statusInternet;
    scraper.URLToString(url, wstr, &statusInternet);
    wcout << wstr << endl;

    BindStatus statusBind;
    scraper.URLToFile(url, LR"(C:\Users\Claudiu HBann\Downloads\test.txt)", &statusBind);

    wstring fileCache;
    scraper.URLToFileCache(url, fileCache, &statusBind);
    wcout << fileCache << endl;

    return 0;
}
