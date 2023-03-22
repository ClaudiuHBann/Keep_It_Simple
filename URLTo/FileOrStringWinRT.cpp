#include <winrt/windows.foundation.h>
#include <winrt/windows.storage.h>
#include <winrt/windows.web.http.h>
#include <winrt/windows.storage.streams.h>

using namespace winrt;
using namespace Windows;
using namespace Foundation;
using namespace Storage;
using namespace Web::Http;
using namespace Filters;
using namespace Streams;

#include <functional>
#include <filesystem>
#include <variant>
#include <iostream>
#include <fstream>

using namespace std;
using namespace filesystem;

#include <io.h>
#include <fcntl.h>

class Downloader
{
  public:
    Downloader() = default;

    Downloader(const IHttpFilter &filter) : mClient(filter)
    {
    }

    inline IAsyncOperation<hstring> URLToStringAsync(
        const hstring &url,
        const function<void(IAsyncOperationWithProgress<hstring, HttpProgress>, HttpProgress)> &callback =
            HttpProgressDefault)
    {
        auto &&responseAsStringAsync = mClient.GetStringAsync(Uri(url));
        responseAsStringAsync.Progress(callback);
        co_return co_await responseAsStringAsync;
    }

    IAsyncAction URLToFileAsync(const hstring &url, const hstring &file,
                                const function<void(IAsyncOperationWithProgress<IInputStream, HttpProgress>,
                                                    HttpProgress)> &callback = HttpProgressDefault,
                                const uint32_t bufferSize = 8192)
    {
        auto &&streamAsync = mClient.GetInputStreamAsync(Uri(url));
        streamAsync.Progress(callback);
        auto &&stream = co_await streamAsync;

        path filePath(file.c_str());
        create_directories(filePath.parent_path());
        wofstream(file.c_str());

        auto &&storageFile = co_await StorageFile::GetFileFromPathAsync(file);
        auto &&fileStream = co_await storageFile.OpenAsync(FileAccessMode::ReadWrite);
        Buffer buffer(numeric_limits<uint16_t>::max());

        co_await stream.ReadAsync(buffer, buffer.Capacity(), InputStreamOptions::None);
        while (buffer.Length())
        {
            co_await fileStream.WriteAsync(buffer);
            co_await stream.ReadAsync(buffer, buffer.Capacity(), InputStreamOptions::None);
        }
    }

  private:
    HttpClient mClient;

    static inline hstring HttpProgressStageValueToString(HttpProgressStage stage)
    {
        switch (stage)
        {
        case HttpProgressStage::None:
            return L"None";
        case HttpProgressStage::DetectingProxy:
            return L"DetectingProxy";
        case HttpProgressStage::ResolvingName:
            return L"ResolvingName";
        case HttpProgressStage::ConnectingToServer:
            return L"ConnectingToServer";
        case HttpProgressStage::NegotiatingSsl:
            return L"NegotiatingSsl";
        case HttpProgressStage::SendingHeaders:
            return L"SendingHeaders";
        case HttpProgressStage::SendingContent:
            return L"SendingContent";
        case HttpProgressStage::WaitingForResponse:
            return L"WaitingForResponse";
        case HttpProgressStage::ReceivingHeaders:
            return L"ReceivingHeaders";
        case HttpProgressStage::ReceivingContent:
            return L"ReceivingContent";
        default:
            return L"Unknown";
        }
    }

    static inline const function<void(variant<IAsyncOperationWithProgress<hstring, HttpProgress>,
                                              IAsyncOperationWithProgress<IInputStream, HttpProgress>>,
                                      HttpProgress)>
        HttpProgressDefault = [](const auto &, const auto &progress) {
            wcout << format(L"Stage: {}", HttpProgressStageValueToString(progress.Stage).c_str()) << endl;
            wcout << format(L"Sent: {} of {} bytes", progress.BytesSent, progress.TotalBytesToSend.GetUInt64()) << endl;
            wcout << format(L"Received: {} of {} bytes", progress.BytesReceived,
                            progress.TotalBytesToReceive ? to_hstring(progress.TotalBytesToReceive.GetUInt64()).c_str()
                                                         : L"?")
                  << endl;
            wcout << format(L"Retries: {}", progress.Retries) << endl << endl;
        };
};

int main()
{
    init_apartment();

    auto &&_ = _setmode(_fileno(stdout), _O_U16TEXT);
    _;

    Downloader downloader;
    hstring url(L"https://github.com/ClaudiuHBann?tab=repositories");

    auto &&hstr = downloader.URLToStringAsync(url).get();
    wcout << hstr.c_str() << endl;

    auto &&folderDownloadsPath = KnownFolders::GetFolderAsync(KnownFolderId::DownloadsFolder).get().Path();
    downloader.URLToFileAsync(url, folderDownloadsPath + LR"(\test.txt)").get();

    clear_factory_cache();
    uninit_apartment();

    return 0;
}
