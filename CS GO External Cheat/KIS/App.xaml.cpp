#include "pch.h"

#include "App.xaml.h"
#include "MainWindow.xaml.h"

namespace winrt::KIS::implementation
{
App::App()
{
    InitializeComponent();

#if defined _DEBUG && !defined DISABLE_XAML_GENERATED_BREAK_ON_UNHANDLED_EXCEPTION
    UnhandledException([this](IInspectable const &, UnhandledExceptionEventArgs const &e) {
        if (IsDebuggerPresent())
        {
            auto errorMessage = e.Message();
            __debugbreak();
        }
    });
#endif
}

void App::OnLaunched(LaunchActivatedEventArgs const &)
{
    window = make<implementation::MainWindow>();
    window.Activate();
}
} // namespace winrt::KIS::implementation
