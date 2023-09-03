#pragma once

#include <Unknwn.h>

// Undefine GetCurrentTime macro to prevent
// conflict with Storyboard::GetCurrentTime
#undef GetCurrentTime

// windows
#include <winrt/Windows.Foundation.h>

using namespace winrt;

using namespace Windows::Foundation;

// microsoft
#include <winrt/Microsoft.UI.Xaml.Markup.h>

using namespace Microsoft::UI::Xaml;
