#pragma once

#include "MainWindow.g.h"

namespace winrt::KIS::implementation
{
struct MainWindow : MainWindowT<MainWindow>
{
    MainWindow();
};
} // namespace winrt::KIS::implementation

namespace winrt::KIS::factory_implementation
{
struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
{
};
} // namespace winrt::KIS::factory_implementation
