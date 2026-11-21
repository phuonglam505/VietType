/*----------------------------------------------------------
OpenKey

Copyright (C) 2019 Mai Vu Tuyen

This file is belong to the OpenKey project, Win32 version
which is released under GPL license.
You can fork, modify, improve this program. If you
redistribute your new version, it MUST be open source.
-----------------------------------------------------------*/
#pragma once
#include "stdafx.h"

class SystemTrayHelper {
private:
	static HWND createFakeWindow(const HINSTANCE& hIns);
	static void createPopupMenu();

public:
    static HWND findRunningWindow();
	static bool createSystemTrayIcon(const HINSTANCE& hIns);
	static void updateData();
    static void refreshLanguage();
	static void removeSystemTray();
};

