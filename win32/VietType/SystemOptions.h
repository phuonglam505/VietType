// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "stdafx.h"

namespace viettype {
int showPanelAtLaunch();
int runElevated();
int checkUpdatesAtLaunch();
bool isElevated();
bool startElevated(LPCWSTR arguments);
bool startsWithWindows();
bool setStartsWithWindows(bool enabled);
bool hasDesktopShortcut();
bool setDesktopShortcut(bool enabled);
constexpr UINT updateResultMessage = WM_APP + 44;
void checkForUpdate(HWND parent, bool interactive);
void onUpdateResult(HWND parent, WPARAM result);
}
