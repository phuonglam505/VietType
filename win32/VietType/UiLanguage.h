// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <windows.h>

namespace viettype {
void loadUiLanguage();
bool isUiEnglish();
void setUiEnglish(bool enabled);
LPCWSTR tr(LPCWSTR vietnamese);
void localizeDialog(HWND window);
}
