// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <array>
#include "BaseDialog.h"

class AppDelegate;

class MainWindow final : public BaseDialog {
public:
    MainWindow(HINSTANCE instance, AppDelegate& controller);
    void fillData() override;
    void refreshInputControls();

private:
    static INT_PTR CALLBACK pageProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK licenseProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
    INT_PTR eventProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) override;
    void init();
    void restoreDefaults();
    void changeHotkey(int control);
    HWND item(int id) const;
    AppDelegate& controller_;
    int iconMode_ = -1;
    std::array<HWND, 4> pages_{};
};
