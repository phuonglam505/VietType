/*----------------------------------------------------------
OpenKey

Copyright (C) 2019 Mai Vu Tuyen

This file is belong to the OpenKey project, Win32 version
which is released under GPL license.
You can fork, modify, improve this program. If you
redistribute your new version, it MUST be open source.
-----------------------------------------------------------*/
#include "SystemTrayHelper.h"
#include "AppDelegate.h"
#include "UiLanguage.h"
#include <oleacc.h>
#include <utility>

#define WM_TRAYMESSAGE (WM_USER + 1)
constexpr UINT showVietTypeTrayMenu = WM_APP + 34;
#define TRAY_ICONUID 100

#define POPUP_VIET_ON_OFF 900

#define POPUP_TELEX 910
#define POPUP_VNI 911
#define POPUP_SIMPLE_TELEX 912
#define POPUP_BILINGUAL_TELEX 913
#define POPUP_VNI_TELEX 914
#define POPUP_BILINGUAL_VNI_TELEX 915

#define POPUP_UNICODE 930
#define POPUP_TCVN3 931
#define POPUP_VNI_WINDOWS 932
#define POPUP_UNICODE_COMPOUND 933
#define POPUP_VN_LOCALE_1258 934
#define POPUP_OTHER_CODES 935
constexpr UINT firstExtendedEncoding = 1100;

#define POPUP_CONVERT_TOOL 980
#define POPUP_QUICK_CONVERT 981

#define POPUP_MACRO_TABLE 990

#define POPUP_CONTROL_PANEL 1000
#define POPUP_VIETTYPE_EXIT 2000

static HMENU popupMenu;
//static HMENU menuInputType;
static HMENU otherCode;

static NOTIFYICONDATA nid;

map<UINT, LPCTSTR> menuData = {
	{POPUP_VIET_ON_OFF, _T("Bật Tiếng Việt")},
	{POPUP_TELEX, _T("Kiểu gõ Telex")},
	{POPUP_VNI, _T("Kiểu gõ VNI")},
	{POPUP_SIMPLE_TELEX, _T("Kiểu gõ Simple Telex")},
	{POPUP_BILINGUAL_TELEX, _T("Telex - giữ từ tiếng Anh")},
	{POPUP_VNI_TELEX, _T("VNI + Telex")},
	{POPUP_BILINGUAL_VNI_TELEX, _T("VNI + Telex - giữ từ tiếng Anh")},
	{POPUP_UNICODE, _T("Unicode dựng sẵn")},
	{POPUP_TCVN3, _T("TCVN3 (ABC)")},
	{POPUP_VNI_WINDOWS, _T("VNI Windows")},
	{POPUP_UNICODE_COMPOUND, _T("Unicode tổ hợp")},
	{POPUP_VN_LOCALE_1258, _T("Vietnamese locale CP 1258")},
	{POPUP_CONVERT_TOOL, _T("Công cụ chuyển mã...")},
	{POPUP_QUICK_CONVERT, _T("Chuyển mã nhanh")},
	{POPUP_MACRO_TABLE, _T("Gõ tắt")},
	{POPUP_CONTROL_PANEL, _T("Mở VietType")},
	{POPUP_VIETTYPE_EXIT, _T("Thoát")},
};

struct TrayMenuLabel {
    MSAAMENUINFO accessibility{};
    std::wstring text;

    void set(std::wstring value) {
        text = std::move(value);
        accessibility = {MSAA_MENU_SIG, static_cast<DWORD>(text.size()), text.data()};
    }
};

static std::map<UINT, TrayMenuLabel> trayLabels;
static HFONT trayMenuFont;

static void attachTrayLabels(HMENU menu) {
    for (int index = 0; index < GetMenuItemCount(menu); ++index) {
        MENUITEMINFO item{};
        item.cbSize = sizeof(item);
        item.fMask = MIIM_FTYPE | MIIM_ID | MIIM_SUBMENU;
        if (!GetMenuItemInfo(menu, index, TRUE, &item) || (item.fType & MFT_SEPARATOR)) continue;
        const UINT id = item.hSubMenu ? POPUP_OTHER_CODES : item.wID;
        auto& label = trayLabels[id];
        label.set(viettype::tr(item.hSubMenu ? L"Bảng mã khác" : menuData.at(id)));
        item.fMask = MIIM_FTYPE | MIIM_DATA;
        item.fType |= MFT_OWNERDRAW;
        item.dwItemData = reinterpret_cast<ULONG_PTR>(&label.accessibility);
        SetMenuItemInfo(menu, index, TRUE, &item);
        if (item.hSubMenu) attachTrayLabels(item.hSubMenu);
    }
}

static HFONT selectTrayMenuFont(HDC dc) {
    return static_cast<HFONT>(SelectObject(dc, trayMenuFont ? trayMenuFont : GetStockObject(DEFAULT_GUI_FONT)));
}

static bool measureTrayItem(HWND owner, MEASUREITEMSTRUCT& item) {
    if (item.CtlType != ODT_MENU || !item.itemData) return false;
    const auto& label = *reinterpret_cast<const MSAAMENUINFO*>(item.itemData);
    const auto dc = GetDC(owner);
    if (!dc) return false;
    const auto originalFont = selectTrayMenuFont(dc);
    SIZE bounds{};
    GetTextExtentPoint32W(dc, label.pszWText, static_cast<int>(label.cchWText), &bounds);
    SelectObject(dc, originalFont);
    ReleaseDC(owner, dc);
    const auto dpi = GetDpiForWindow(owner);
    item.itemWidth = bounds.cx + MulDiv(37, dpi, 96);
    item.itemHeight = std::max<LONG>(bounds.cy + MulDiv(9, dpi, 96), GetSystemMetrics(SM_CYMENU));
    return true;
}

static bool paintTrayItem(HWND owner, const DRAWITEMSTRUCT& item) {
    if (item.CtlType != ODT_MENU || !item.itemData) return false;
    const auto& label = *reinterpret_cast<const MSAAMENUINFO*>(item.itemData);
    const bool selected = (item.itemState & (ODS_SELECTED | ODS_HOTLIGHT)) != 0;
    const bool disabled = (item.itemState & (ODS_DISABLED | ODS_GRAYED)) != 0;
    FillRect(item.hDC, &item.rcItem, GetSysColorBrush(selected ? COLOR_HIGHLIGHT : COLOR_MENU));
    const auto originalFont = selectTrayMenuFont(item.hDC);
    const auto textColor = SetTextColor(item.hDC, GetSysColor(
        selected ? COLOR_HIGHLIGHTTEXT : disabled ? COLOR_GRAYTEXT : COLOR_MENUTEXT));
    const auto background = SetBkMode(item.hDC, TRANSPARENT);
    const auto dpi = GetDpiForWindow(owner);
    auto text = item.rcItem;
    text.left += MulDiv(23, dpi, 96);
    text.right -= MulDiv(14, dpi, 96);
    constexpr UINT flags = DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX;
    DrawTextW(item.hDC, label.pszWText, static_cast<int>(label.cchWText), &text, flags);
    if (item.itemState & ODS_CHECKED) {
        auto check = item.rcItem;
        check.right = text.left;
        DrawTextW(item.hDC, L"✓", 1, &check, flags | DT_CENTER);
    }
    SetBkMode(item.hDC, background);
    SetTextColor(item.hDC, textColor);
    SelectObject(item.hDC, originalFont);
    return true;
}

HWND SystemTrayHelper::findRunningWindow() {
    if (const HWND current = FindWindow(APP_CLASS, nullptr)) return current;
    // Earlier VietType builds used a different tray-window class.
    return FindWindow(L"VietTypeOpenKeyVietnameseInputMethod", nullptr);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	static UINT taskbarCreated;

	switch (message) {
	case WM_CREATE:
		taskbarCreated = RegisterWindowMessage(_T("TaskbarCreated"));
		break;
	case queryVietTypeBuild:
		return VIETTYPE_VERSION_NUMBER;
	case showVietTypeWindow:
		AppDelegate::getInstance()->onControlPanel();
		break;
    case WM_TIMER:
        if (wParam == 1) VietTypeManager::refreshImeStatus();
        return 0;
    case inputModeChanged: {
        auto& app = *AppDelegate::getInstance();
        app.onInputMethodChangedFromHotKey();
        if (vUseSmartSwitchKey && reinterpret_cast<HWND>(lParam) == GetForegroundWindow()) {
            setAppInputMethodStatus(VietTypeHelper::getFrontMostAppExecuteName(), static_cast<int>(wParam));
            saveSmartSwitchKeyData();
        }
        return 0;
    }
    case quickConvertRequested:
        AppDelegate::getInstance()->onQuickConvert();
        return 0;
    case inputDeliveryFailed: {
        static ULONGLONG lastNotice = 0;
        const auto now = GetTickCount64();
        if (!lastNotice || now - lastNotice >= 10000) {
            lastNotice = now;
            auto warning = nid;
            warning.uFlags = NIF_INFO;
            wcscpy_s(warning.szInfoTitle, L"VietType");
            wcscpy_s(warning.szInfo, viettype::tr(L"Không gửi được chữ tiếng Việt. VietType đã chuyển tiếp phím gốc."));
            warning.dwInfoFlags = NIIF_WARNING;
            Shell_NotifyIcon(NIM_MODIFY, &warning);
        }
        return 0;
    }
    case WM_MEASUREITEM:
        if (measureTrayItem(hWnd, *reinterpret_cast<MEASUREITEMSTRUCT*>(lParam))) return TRUE;
        break;
    case WM_DRAWITEM:
        if (paintTrayItem(hWnd, *reinterpret_cast<const DRAWITEMSTRUCT*>(lParam))) return TRUE;
        break;
    case WM_TRAYMESSAGE:
        if (lParam == WM_RBUTTONUP) {
            PostMessage(hWnd, showVietTypeTrayMenu, 0, 0);
        } else if (lParam == WM_LBUTTONDBLCLK) {
            AppDelegate::getInstance()->onControlPanel();
        } else if (lParam == WM_LBUTTONUP) {
            AppDelegate::getInstance()->onToggleVietnamese();
        }
        return 0;
    case showVietTypeTrayMenu: {
        POINT position{};
        if (!popupMenu) return 0;
        if (!GetCursorPos(&position)) {
            const DWORD messagePosition = GetMessagePos();
            const POINTS lastPosition = MAKEPOINTS(messagePosition);
            position = {lastPosition.x, lastPosition.y};
        }
        const UINT alignment = GetSystemMetrics(SM_MENUDROPALIGNMENT) ? TPM_RIGHTALIGN : TPM_LEFTALIGN;
        // A shown tool window gives the popup a foreground owner while it tracks the mouse.
        SetWindowPos(hWnd, HWND_TOP, position.x, position.y, 1, 1, SWP_SHOWWINDOW | SWP_NOACTIVATE);
        SetForegroundWindow(hWnd);
        NONCLIENTMETRICSW metrics{};
        metrics.cbSize = sizeof(metrics);
        if (SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0, GetDpiForWindow(hWnd))) {
            trayMenuFont = CreateFontIndirectW(&metrics.lfMenuFont);
        }
        const UINT commandId = TrackPopupMenu(
            popupMenu, alignment | TPM_RIGHTBUTTON | TPM_RETURNCMD,
            position.x, position.y, 0, hWnd, nullptr);
        if (trayMenuFont) {
            DeleteObject(trayMenuFont);
            trayMenuFont = nullptr;
        }
        ShowWindow(hWnd, SW_HIDE);
        PostMessage(hWnd, WM_NULL, 0, 0);
        if (commandId == 0) {
            Shell_NotifyIcon(NIM_SETFOCUS, &nid);
            return 0;
        }
        const auto& encodings = VietTypeManager::getTableCode();
        if (commandId >= firstExtendedEncoding && commandId < firstExtendedEncoding + encodings.size() - 5) {
            AppDelegate::getInstance()->onTableCode(5 + commandId - firstExtendedEncoding);
            return 0;
        }
        switch (commandId) {
        case POPUP_VIET_ON_OFF:
        	AppDelegate::getInstance()->onToggleVietnamese();
        	break;

        case POPUP_MACRO_TABLE:
        	AppDelegate::getInstance()->onMacroTable();
        	break;
        case POPUP_CONVERT_TOOL:
        	AppDelegate::getInstance()->onConvertTool();
        	break;
        case POPUP_QUICK_CONVERT:
        	AppDelegate::getInstance()->onQuickConvert();
        	break;
        case POPUP_TELEX:
        	AppDelegate::getInstance()->onInputType(0);
        	break;
        case POPUP_VNI:
        	AppDelegate::getInstance()->onInputType(1);
        	break;
        case POPUP_SIMPLE_TELEX:
        	AppDelegate::getInstance()->onInputType(2);
        	break;
        case POPUP_BILINGUAL_TELEX:
            AppDelegate::getInstance()->onInputType(vBilingualTelex);
            break;
        case POPUP_VNI_TELEX:
            AppDelegate::getInstance()->onInputType(vVniTelex);
            break;
        case POPUP_BILINGUAL_VNI_TELEX:
            AppDelegate::getInstance()->onInputType(vBilingualVniTelex);
            break;
        case POPUP_UNICODE:
        	AppDelegate::getInstance()->onTableCode(0);
        	break;
        case POPUP_TCVN3:
        	AppDelegate::getInstance()->onTableCode(1);
        	break;
        case POPUP_VNI_WINDOWS:
        	AppDelegate::getInstance()->onTableCode(2);
        	break;
        case POPUP_UNICODE_COMPOUND:
        	AppDelegate::getInstance()->onTableCode(3);
        	break;
        case POPUP_VN_LOCALE_1258:
        	AppDelegate::getInstance()->onTableCode(4);
        	break;

        case POPUP_CONTROL_PANEL:
        	AppDelegate::getInstance()->onControlPanel();
        	break;

        case POPUP_VIETTYPE_EXIT:
        	AppDelegate::getInstance()->onVietTypeExit();
        	break;
        }
        SystemTrayHelper::updateData();
        return 0;
	}
	default:
		// if the taskbar is restarted, add the system tray icon again
		if (message == taskbarCreated) {
			Shell_NotifyIcon(NIM_ADD, &nid);
		}
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

HWND SystemTrayHelper::createFakeWindow(const HINSTANCE & hIns) {
	//create fake window
	WNDCLASSEXW wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = 0;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hIns;
	wcex.hIcon = LoadIcon(hIns, MAKEINTRESOURCE(IDI_APP_ICON));
	wcex.hCursor = NULL;
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = APP_CLASS;
	wcex.hIconSm = NULL;
	ATOM atom = RegisterClassExW(&wcex);
	HWND hWnd = CreateWindowExW(WS_EX_TOOLWINDOW, APP_CLASS, L"", WS_POPUP,
		0, 0, 1, 1, nullptr, nullptr, hIns, nullptr);
	if (!hWnd) {
		return NULL;
	}
	ShowWindow(hWnd, 0);
	UpdateWindow(hWnd);
	return hWnd;
}

void SystemTrayHelper::createPopupMenu() {
	popupMenu = CreatePopupMenu();
	AppendMenu(popupMenu, MF_CHECKED, POPUP_VIET_ON_OFF, menuData[POPUP_VIET_ON_OFF]);
	AppendMenu(popupMenu, MF_SEPARATOR, 0, 0);
	AppendMenu(popupMenu, MF_UNCHECKED, POPUP_MACRO_TABLE, menuData[POPUP_MACRO_TABLE]);
	AppendMenu(popupMenu, MF_UNCHECKED, POPUP_CONVERT_TOOL, menuData[POPUP_CONVERT_TOOL]);
	AppendMenu(popupMenu, MF_UNCHECKED, POPUP_QUICK_CONVERT, menuData[POPUP_QUICK_CONVERT]);
	AppendMenu(popupMenu, MF_SEPARATOR, 0, 0);

	//menuInputType = CreatePopupMenu();
	AppendMenu(popupMenu, MF_STRING, POPUP_VNI, menuData[POPUP_VNI]);
	AppendMenu(popupMenu, MF_STRING, POPUP_BILINGUAL_TELEX, menuData[POPUP_BILINGUAL_TELEX]);
	AppendMenu(popupMenu, MF_STRING, POPUP_TELEX, menuData[POPUP_TELEX]);
	AppendMenu(popupMenu, MF_STRING, POPUP_SIMPLE_TELEX, menuData[POPUP_SIMPLE_TELEX]);
	AppendMenu(popupMenu, MF_STRING, POPUP_VNI_TELEX, menuData[POPUP_VNI_TELEX]);
	AppendMenu(popupMenu, MF_STRING, POPUP_BILINGUAL_VNI_TELEX, menuData[POPUP_BILINGUAL_VNI_TELEX]);

	//AppendMenu(popupMenu, MF_POPUP, (UINT_PTR)menuInputType, _T("Kiểu gõ"));
	AppendMenu(popupMenu, MF_SEPARATOR, 0, 0);

	AppendMenu(popupMenu, MF_UNCHECKED, POPUP_UNICODE, menuData[POPUP_UNICODE]);
	AppendMenu(popupMenu, MF_UNCHECKED, POPUP_TCVN3, menuData[POPUP_TCVN3]);
	AppendMenu(popupMenu, MF_UNCHECKED, POPUP_VNI_WINDOWS, menuData[POPUP_VNI_WINDOWS]);

	otherCode = CreatePopupMenu();
	AppendMenu(otherCode, MF_CHECKED, POPUP_UNICODE_COMPOUND, menuData[POPUP_UNICODE_COMPOUND]);
	AppendMenu(otherCode, MF_CHECKED, POPUP_VN_LOCALE_1258, menuData[POPUP_VN_LOCALE_1258]);
    const auto& encodings = VietTypeManager::getTableCode();
    for (size_t index = 5; index < encodings.size(); ++index) {
        const auto id = firstExtendedEncoding + static_cast<UINT>(index - 5);
        menuData[id] = encodings[index];
        AppendMenu(otherCode, MF_STRING, id, encodings[index]);
    }
	AppendMenu(popupMenu, MF_POPUP, (UINT_PTR)otherCode, _T("Bảng mã khác"));

	AppendMenu(popupMenu, MF_SEPARATOR, 0, 0);

	AppendMenu(popupMenu, MF_STRING, POPUP_CONTROL_PANEL, menuData[POPUP_CONTROL_PANEL]);
	AppendMenu(popupMenu, MF_SEPARATOR, 0, 0);
	AppendMenu(popupMenu, MF_UNCHECKED, POPUP_VIETTYPE_EXIT, menuData[POPUP_VIETTYPE_EXIT]);

	SetMenuDefaultItem(popupMenu, POPUP_CONTROL_PANEL, false);
	attachTrayLabels(popupMenu);
}

static void loadTrayIcon() {
	int icon = 0;
	if (vLanguage) {
		icon = vUseGrayIcon ? IDI_ICON_STATUS_VIET_10 : IDI_ICON_STATUS_VIET;
		wcscpy_s(nid.szTip, viettype::tr(L"Bấm vào đây để tắt gõ Tiếng Việt"));
	}
	else {
		icon = vUseGrayIcon ? IDI_ICON_STATUS_ENG_10 : IDI_ICON_STATUS_ENG;
		wcscpy_s(nid.szTip, viettype::tr(L"Bấm vào đây để bật gõ Tiếng Việt"));
	}
	nid.hIcon = LoadIcon(GetModuleHandle(0), MAKEINTRESOURCE(icon));
}

void SystemTrayHelper::refreshLanguage() {
    if (popupMenu) attachTrayLabels(popupMenu);
    updateData();
}

void SystemTrayHelper::updateData() {
	loadTrayIcon();
	Shell_NotifyIcon(NIM_MODIFY, &nid);

	CheckMenuItem(popupMenu, POPUP_VIET_ON_OFF, MF_BYCOMMAND | ((vLanguage) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(popupMenu, POPUP_TELEX, MF_BYCOMMAND | ((vInputType == 0) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(popupMenu, POPUP_VNI, MF_BYCOMMAND | ((vInputType == 1) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(popupMenu, POPUP_SIMPLE_TELEX, MF_BYCOMMAND | ((vInputType == vSimpleTelex1 || vInputType == vSimpleTelex2) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(popupMenu, POPUP_BILINGUAL_TELEX, MF_BYCOMMAND | (vInputType == vBilingualTelex ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(popupMenu, POPUP_VNI_TELEX, MF_BYCOMMAND | (vInputType == vVniTelex ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(popupMenu, POPUP_BILINGUAL_VNI_TELEX, MF_BYCOMMAND | (vInputType == vBilingualVniTelex ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(popupMenu, POPUP_UNICODE, MF_BYCOMMAND | ((vCodeTable == 0) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(popupMenu, POPUP_TCVN3, MF_BYCOMMAND | ((vCodeTable == 1) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(popupMenu, POPUP_VNI_WINDOWS, MF_BYCOMMAND | ((vCodeTable == 2) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(otherCode, POPUP_UNICODE_COMPOUND, MF_BYCOMMAND | ((vCodeTable == 3) ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(otherCode, POPUP_VN_LOCALE_1258, MF_BYCOMMAND | ((vCodeTable == 4) ? MF_CHECKED : MF_UNCHECKED));

    for (size_t index = 5; index < VietTypeManager::getTableCode().size(); ++index) {
        const auto id = firstExtendedEncoding + static_cast<UINT>(index - 5);
        CheckMenuItem(otherCode, id, MF_BYCOMMAND | (vCodeTable == index ? MF_CHECKED : MF_UNCHECKED));
    }

	wstring hotkey = L"";
	bool hasAdd = false;
	if (convertToolHotKey & 0x100) {
		hotkey += L"Ctrl";
		hasAdd = true;
	}
	if (convertToolHotKey & 0x200) {
		if (hasAdd)
			hotkey += L" + ";
		hotkey += L"Alt";
		hasAdd = true;
	}
	if (convertToolHotKey & 0x400) {
		if (hasAdd)
			hotkey += L" + ";
		hotkey += L"Win";
		hasAdd = true;
	}
	if (convertToolHotKey & 0x800) {
		if (hasAdd)
			hotkey += L" + ";
		hotkey += L"Shift";
		hasAdd = true;
	}

	unsigned short k = ((convertToolHotKey >> 24) & 0xFF);
	if (k != 0xFE) {
		if (hasAdd)
			hotkey += L" + ";
		if (k == VK_SPACE)
			hotkey += L"Space";
		else
			hotkey += (wchar_t)k;
	}

	wstring hotKeyString = viettype::tr(menuData[POPUP_QUICK_CONVERT]);
	if (hasAdd) {
		hotKeyString += L" - [";
		hotKeyString += hotkey;
		hotKeyString += L"]";
	}
	if (const auto label = trayLabels.find(POPUP_QUICK_CONVERT); label != trayLabels.end()) {
        label->second.set(std::move(hotKeyString));
    }
}

bool SystemTrayHelper::createSystemTrayIcon(const HINSTANCE& instance) {
    const HWND window = createFakeWindow(instance);
    if (!window) return false;
    createPopupMenu();
    nid.cbSize = sizeof(nid);
    nid.hWnd = window;
    nid.uID = TRAY_ICONUID;
    nid.uCallbackMessage = WM_TRAYMESSAGE;
    nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    loadTrayIcon();
    SetTimer(window, 1, 250, nullptr);
    for (int attempt = 0; attempt < 5; ++attempt) {
        if (Shell_NotifyIcon(NIM_ADD, &nid)) return true;
        if (attempt < 4) Sleep(500);
    }
    return false;
}

void SystemTrayHelper::removeSystemTray() {
	Shell_NotifyIcon(NIM_DELETE, &nid);
    if (nid.hWnd) DestroyWindow(nid.hWnd);
}