/*----------------------------------------------------------
OpenKey

Copyright (C) 2019 Mai Vu Tuyen

This file is belong to the OpenKey project, Win32 version
which is released under GPL license.
You can fork, modify, improve this program. If you
redistribute your new version, it MUST be open source.
-----------------------------------------------------------*/
#include "stdafx.h"
#include "AppDelegate.h"
#include "SystemOptions.h"
#include "SelfUpdate.h"

#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, LPWSTR arguments, int)
{
    if (!viettype::completeUpdate(arguments)) return 1;
    const HWND existing = SystemTrayHelper::findRunningWindow();
    if (!existing) VietTypeHelper::migratePreviousSettings();
    if (!existing && viettype::runElevated() && !viettype::isElevated()) {
        if (viettype::startElevated(wcscmp(arguments, L"--startup") == 0 ? L"--startup" : nullptr)) {
            return 0;
        }
        MessageBox(nullptr, L"VietType sẽ chạy với quyền thông thường.",
                   L"VietType", MB_OK | MB_ICONINFORMATION);
    }
    INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_WIN95_CLASSES};
    InitCommonControlsEx(&controls);
    AppDelegate app;
    return app.run(instance);
}
