// SPDX-License-Identifier: GPL-3.0-or-later
#include "MainWindow.h"
#include "AppDelegate.h"
#include "SystemOptions.h"
#include <uxtheme.h>

namespace {
struct Toggle {
    int id;
    LPCWSTR key;
    int* value;
    int defaultValue;
    bool inverted;
};

const Toggle toggles[] = {
    {IDC_PREF_ORTHO, L"vUseModernOrthography", &vUseModernOrthography, 0, false},
    {IDC_PREF_SPELL, L"vCheckSpelling", &vCheckSpelling, 1, false},
    {IDC_PREF_BROWSER, L"vFixRecommendBrowser", &vFixRecommendBrowser, 1, false},
    {IDC_PREF_RESTORE, L"vRestoreIfWrongSpelling", &vRestoreIfWrongSpelling, 1, false},
    {IDC_PREF_CAPS, L"vUpperCaseFirstChar", &vUpperCaseFirstChar, 0, false},
    {IDC_PREF_LETTERS, L"vAllowConsonantZFWJ", &vAllowConsonantZFWJ, 0, false},
    {IDC_PREF_SMART, L"vUseSmartSwitchKey", &vUseSmartSwitchKey, 1, false},
    {IDC_PREF_CTRL_PAUSE, L"vTempOffSpelling", &vTempOffSpelling, 0, false},
    {IDC_PREF_REMEMBER, L"vRememberCode", &vRememberCode, 1, false},
    {IDC_PREF_ALT_PAUSE, L"vTempOffVietType", &vTempOffVietType, 0, false},
    {IDC_PREF_ENABLE_MACRO, L"vUseMacro", &vUseMacro, 1, false},
    {IDC_PREF_QUICK, L"vQuickTelex", &vQuickTelex, 0, false},
    {IDC_PREF_EN_MACRO, L"vUseMacroInEnglishMode", &vUseMacroInEnglishMode, 0, false},
    {IDC_PREF_START_CONS, L"vQuickStartConsonant", &vQuickStartConsonant, 0, false},
    {IDC_PREF_MACRO_CAPS, L"vAutoCapsMacro", &vAutoCapsMacro, 0, false},
    {IDC_PREF_END_CONS, L"vQuickEndConsonant", &vQuickEndConsonant, 0, false},
    {IDC_PREF_METRO, L"vSupportMetroApp", &vSupportMetroApp, 0, false},
    {IDC_PREF_CHROMIUM, L"vFixChromiumBrowser", &vFixChromiumBrowser, 0, false},
    {IDC_PREF_SHOW, L"showPanelAtLaunch", nullptr, 1, false},
    {IDC_PREF_GRAY, L"vUseGrayIcon", &vUseGrayIcon, 0, false},
    {IDC_PREF_CLIPBOARD, L"vSendKeyStepByStep", &vSendKeyStepByStep, 1, true},
    {IDC_PREF_ADMIN, L"runElevated", nullptr, 0, false},
    {IDC_PREF_AUTO_UPDATE, L"checkUpdatesAtLaunch", nullptr, 1, false},
};

struct HotkeyFlag { int id; unsigned mask; };
constexpr HotkeyFlag hotkeyFlags[] = {
    {IDC_PREF_CTRL, 0x100},
    {IDC_PREF_ALT, 0x200},
    {IDC_PREF_WIN, 0x400},
    {IDC_PREF_SHIFT, 0x800},
    {IDC_PREF_BEEP, 0x8000},
};
constexpr unsigned defaultHotkey = 0x5a00025a;
constexpr unsigned noPrimaryKey = 0xfe;

void setToggle(const Toggle& option, int value)
{
    if (option.value) *option.value = value;
    VietTypeHelper::setRegInt(option.key, value);
}

void reportWindowsError(HWND parent)
{
    MessageBox(parent, L"Windows không lưu được tùy chọn này.",
               L"VietType", MB_OK | MB_ICONWARNING);
}
}

MainWindow::MainWindow(HINSTANCE instance, AppDelegate& controller)
    : BaseDialog(instance, IDD_VIETTYPE), controller_(controller)
{
}

HWND MainWindow::item(int id) const
{
    if (const auto control = GetDlgItem(hDlg, id)) return control;
    for (const auto page : pages_) {
        if (const auto item = GetDlgItem(page, id)) return item;
    }
    return nullptr;
}

void MainWindow::init()
{
    constexpr int resources[] = {IDD_VT_PAGE_INPUT, IDD_VT_PAGE_MACROS, IDD_VT_PAGE_WINDOWS, IDD_VT_PAGE_INFO};
    constexpr LPCWSTR labels[] = {L"Bộ gõ", L"Gõ tắt", L"Hệ thống", L"Thông tin"};
    const auto tabs = item(IDC_PREF_NAV);
    for (size_t i = 0; i < pages_.size(); ++i) {
        TCITEM tab{};
        tab.mask = TCIF_TEXT;
        tab.pszText = const_cast<LPWSTR>(labels[i]);
        TabCtrl_InsertItem(tabs, i, &tab);
    }
    RECT location{};
    GetClientRect(tabs, &location);
    TabCtrl_AdjustRect(tabs, FALSE, &location);
    MapWindowPoints(tabs, hDlg, reinterpret_cast<POINT*>(&location), 2);
    for (size_t i = 0; i < pages_.size(); ++i) {
        pages_[i] = CreateDialogParam(hInstance, MAKEINTRESOURCE(resources[i]), hDlg, pageProc, 0);
        if (!pages_[i]) continue;
        EnableThemeDialogTexture(pages_[i], ETDT_ENABLETAB);
        SetWindowPos(pages_[i], HWND_TOP, location.left, location.top,
                     location.right - location.left, location.bottom - location.top, SWP_NOACTIVATE);
    }
    TabCtrl_SetCurSel(tabs, 0);
    ShowWindow(pages_[0], SW_SHOWNA);

    const auto populate = [this](int id, const std::vector<LPCTSTR>& entries) {
        for (const auto* label : entries) {
            SendMessage(item(id), CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label));
        }
    };
    populate(IDC_VT_METHOD, VietTypeManager::getInputType());
    populate(IDC_VT_CHARSET, VietTypeManager::getTableCode());

    const auto keys = item(IDC_PREF_KEYCODE);
    const auto addKey = [keys](LPCWSTR label, unsigned key) {
        const LRESULT row = SendMessage(keys, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label));
        if (row >= 0) SendMessage(keys, CB_SETITEMDATA, row, key);
    };
    addKey(L"Chỉ dùng phím kết hợp", noPrimaryKey);
    addKey(L"Space", VK_SPACE);
    for (wchar_t ch : std::wstring(L"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789")) {
        const wchar_t label[] = {ch, 0};
        addKey(label, ch);
    }
    for (unsigned key = VK_F1; key <= VK_F12; ++key) {
        const auto label = L"F" + std::to_wstring(key - VK_F1 + 1);
        addKey(label.c_str(), key);
    }
    const auto savedKey = static_cast<unsigned>(vSwitchKeyStatus) >> 24;
    bool listed = false;
    for (int i = 0; i < SendMessage(keys, CB_GETCOUNT, 0, 0); ++i) {
        if (SendMessage(keys, CB_GETITEMDATA, i, 0) == savedKey) listed = true;
    }
    if (!listed) {
        const auto label = L"Phím đã lưu (" + std::to_wstring(savedKey) + L")";
        addKey(label.c_str(), savedKey);
    }

    SetWindowText(item(IDC_PREF_INFO_TEXT),
        L"VietType " VIETTYPE_VERSION_DISPLAY L" cho Windows x64\r\n\r\n"
        L"Bộ gõ tiếng Việt VietType\r\n"
        L"Phát triển theo giấy phép GNU GPL v3.");
    fillData();
}

void MainWindow::refreshInputControls()
{
    const int mode = vLanguage != 0;
    if (mode != iconMode_) {
        iconMode_ = mode;
        const auto resource = MAKEINTRESOURCE(mode ? IDI_ICON_STATUS_VIET : IDI_ICON_STATUS_ENG);
        const auto loadIcon = [this, resource](int width, int height) {
            return reinterpret_cast<LPARAM>(
                LoadImage(hInstance, resource, IMAGE_ICON, width, height, LR_SHARED));
        };
        SendMessage(hDlg, WM_SETICON, ICON_SMALL, loadIcon(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON)));
        const auto largeIcon = loadIcon(GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON));
        SendMessage(hDlg, WM_SETICON, ICON_BIG, largeIcon);
        SendMessage(item(IDC_VT_MODE_ICON), STM_SETICON, static_cast<WPARAM>(largeIcon), 0);
    }
    CheckDlgButton(hDlg, IDC_VT_LANGUAGE, vLanguage ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_VT_ENGLISH, vLanguage ? BST_UNCHECKED : BST_CHECKED);
    SendMessage(item(IDC_VT_METHOD), CB_SETCURSEL, vInputType, 0);
    SendMessage(item(IDC_VT_CHARSET), CB_SETCURSEL, vCodeTable, 0);
}

void MainWindow::fillData()
{
    refreshInputControls();
    for (const auto& option : toggles) {
        const int value = option.value ? *option.value : VietTypeHelper::getRegInt(option.key, option.defaultValue);
        SendMessage(item(option.id), BM_SETCHECK, (bool(value) != option.inverted) ? BST_CHECKED : BST_UNCHECKED, 0);
    }
    SendMessage(item(IDC_PREF_STARTUP), BM_SETCHECK, viettype::startsWithWindows() ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(item(IDC_PREF_LINK), BM_SETCHECK, viettype::hasDesktopShortcut() ? BST_CHECKED : BST_UNCHECKED, 0);
    for (const auto& flag : hotkeyFlags) {
        SendMessage(item(flag.id), BM_SETCHECK, (vSwitchKeyStatus & flag.mask) ? BST_CHECKED : BST_UNCHECKED, 0);
    }
    const auto keys = item(IDC_PREF_KEYCODE);
    const auto savedKey = static_cast<unsigned>(vSwitchKeyStatus) >> 24;
    for (int i = 0; i < SendMessage(keys, CB_GETCOUNT, 0, 0); ++i) {
        if (SendMessage(keys, CB_GETITEMDATA, i, 0) == savedKey) {
            if (SendMessage(keys, CB_GETCURSEL, 0, 0) != i) SendMessage(keys, CB_SETCURSEL, i, 0);
            break;
        }
    }
}

void MainWindow::changeHotkey(int control)
{
    unsigned status = static_cast<unsigned>(vSwitchKeyStatus);
    if (control == IDC_PREF_KEYCODE) {
        const auto keys = item(control);
        const auto row = SendMessage(keys, CB_GETCURSEL, 0, 0);
        if (row == CB_ERR) return;
        const auto code = static_cast<unsigned>(SendMessage(keys, CB_GETITEMDATA, row, 0));
        status = (status & 0x00ffff00) | (code << 24) | code;
    } else {
        for (const auto& flag : hotkeyFlags) {
            if (flag.id == control) {
                status &= ~flag.mask;
                if (SendMessage(item(control), BM_GETCHECK, 0, 0) == BST_CHECKED) status |= flag.mask;
                break;
            }
        }
    }
    APP_SET_DATA(vSwitchKeyStatus, static_cast<int>(status));
}

void MainWindow::restoreDefaults()
{
    if (MessageBox(hDlg, L"Khôi phục tùy chọn mặc định của VietType?", L"VietType",
                   MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) != IDYES) return;
    for (const auto& option : toggles) setToggle(option, option.defaultValue);
    const bool startupReset = viettype::setStartsWithWindows(false);
    const bool shortcutReset = viettype::setDesktopShortcut(false);
    if (!startupReset || !shortcutReset) reportWindowsError(hDlg);
    APP_SET_DATA(vSwitchKeyStatus, static_cast<int>(defaultHotkey));
    APP_SET_DATA(vLanguage, 1);
    APP_SET_DATA(vInputType, 0);
    APP_SET_DATA(vCodeTable, 0);
    vSetCheckSpelling();
    startNewSession();
    controller_.onInputMethodChangedFromHotKey();
    fillData();
}

INT_PTR MainWindow::eventProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_INITDIALOG) {
        hDlg = window;
        init();
        return TRUE;
    }
    if (message == viettype::updateResultMessage) {
        viettype::onUpdateResult(window, wParam);
        return TRUE;
    }
    if (message == WM_CLOSE || (message == WM_COMMAND && LOWORD(wParam) == IDCANCEL)) {
        ShowWindow(window, SW_HIDE);
        return TRUE;
    }
    if (message == WM_NOTIFY) {
        const auto* notification = reinterpret_cast<const NMHDR*>(lParam);
        if (notification->idFrom == IDC_PREF_NAV && notification->code == TCN_SELCHANGE) {
            const int selected = TabCtrl_GetCurSel(item(IDC_PREF_NAV));
            for (size_t i = 0; i < pages_.size(); ++i) {
                ShowWindow(pages_[i], selected == static_cast<int>(i) ? SW_SHOWNA : SW_HIDE);
            }
            return TRUE;
        }
        return FALSE;
    }
    if (message != WM_COMMAND) return FALSE;
    const int id = LOWORD(wParam);
    if ((id == IDC_VT_METHOD || id == IDC_VT_CHARSET) && HIWORD(wParam) == CBN_SELCHANGE) {
        const auto index = static_cast<int>(SendMessage(item(id), CB_GETCURSEL, 0, 0));
        if (index == CB_ERR) return TRUE;
        if (id == IDC_VT_METHOD) controller_.onInputType(index);
        else controller_.onTableCode(index);
        return TRUE;
    }
    if (id == IDC_PREF_KEYCODE && HIWORD(wParam) == CBN_SELCHANGE) {
        changeHotkey(id);
        return TRUE;
    }
    if (HIWORD(wParam) != BN_CLICKED) return FALSE;
    switch (id) {
        case IDC_VT_LANGUAGE:
        case IDC_VT_ENGLISH:
            if ((id == IDC_VT_LANGUAGE) != (vLanguage != 0)) controller_.onToggleVietnamese();
            return TRUE;
        case IDC_VT_CONVERT: controller_.onConvertTool(); return TRUE;
        case IDC_VT_QUIT: controller_.onVietTypeExit(); return TRUE;
        case IDC_PREF_RESTORE_DEFAULT: restoreDefaults(); return TRUE;
        case IDC_PREF_MENU_MACRO: controller_.onMacroTable(); return TRUE;
        case IDC_PREF_CHECK_UPDATE: viettype::checkForUpdate(window, true); return TRUE;
        case IDC_PREF_LICENSE:
            DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_VT_LICENSE), window, licenseProc, 0);
            return TRUE;
    }
    for (const auto& flag : hotkeyFlags) {
        if (id == flag.id) { changeHotkey(id); return TRUE; }
    }
    const bool checked = SendMessage(item(id), BM_GETCHECK, 0, 0) == BST_CHECKED;
    if (id == IDC_PREF_STARTUP || id == IDC_PREF_LINK) {
        const bool success = id == IDC_PREF_STARTUP ? viettype::setStartsWithWindows(checked) :
                                                        viettype::setDesktopShortcut(checked);
        if (!success) reportWindowsError(window);
        fillData();
        return TRUE;
    }
    for (const auto& option : toggles) {
        if (option.id != id) continue;
        setToggle(option, checked != option.inverted);
        if (id == IDC_PREF_SPELL) vSetCheckSpelling();
        if (option.value) startNewSession();
        if (id == IDC_PREF_GRAY) SystemTrayHelper::updateData();
        return TRUE;
    }
    return FALSE;
}

INT_PTR CALLBACK MainWindow::pageProc(HWND page, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_COMMAND) return SendMessage(GetParent(page), message, wParam, lParam);
    return FALSE;
}

INT_PTR CALLBACK MainWindow::licenseProc(HWND window, UINT message, WPARAM wParam, LPARAM)
{
    if (message == WM_INITDIALOG) {
        SetFocus(GetDlgItem(window, IDCANCEL));
        const auto instance = GetModuleHandle(nullptr);
        const auto resource = FindResource(instance, MAKEINTRESOURCE(IDR_VT_LICENSE), RT_RCDATA);
        if (!resource) return FALSE;
        const auto raw = static_cast<const char*>(LockResource(LoadResource(instance, resource)));
        const auto bytes = SizeofResource(instance, resource);
        if (!raw) return FALSE;
        std::wstring formatted;
        for (DWORD i = 0; i < bytes; ++i) {
            if (raw[i] == '\n' && (i == 0 || raw[i-1] != '\r')) formatted += L'\r';
            formatted += static_cast<unsigned char>(raw[i]);
        }
        SetDlgItemText(window, IDC_PREF_LICENSE_TEXT, formatted.c_str());
        SendDlgItemMessage(window, IDC_PREF_LICENSE_TEXT, EM_SETSEL, 0, 0);
        return FALSE;
    }
    if (message == WM_CLOSE || (message == WM_COMMAND && LOWORD(wParam) == IDCANCEL)) {
        EndDialog(window, 0);
        return TRUE;
    }
    return FALSE;
}
