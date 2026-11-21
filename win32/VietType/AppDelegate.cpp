/*----------------------------------------------------------
OpenKey

Copyright (C) 2019 Mai Vu Tuyen

This file is belong to the OpenKey project, Win32 version
which is released under GPL license.
You can fork, modify, improve this program. If you
redistribute your new version, it MUST be open source.
-----------------------------------------------------------*/
#include "MainWindow.h"
#include "SystemOptions.h"
#include "SelfUpdate.h"
#include "AppDelegate.h"

static AppDelegate* _instance;

//see document in Engine.h
int vLanguage = 1;
int vInputType = 0;
int vFreeMark = 0;
int vCodeTable = 0;
int vCheckSpelling = 1;
int vUseModernOrthography = 1;
int vQuickTelex = 0;
int vSwitchKeyStatus = defaultSwitchHotkey;
int vRestoreIfWrongSpelling = 1;
int vFixRecommendBrowser = 0;
int vUseMacro = 1;
int vUseMacroInEnglishMode = 1;
int vAutoCapsMacro = 0;
int vSendKeyStepByStep = 1;
int vUseSmartSwitchKey = 1;
int vUpperCaseFirstChar = 0;
int vTempOffSpelling = 0;
int vAllowConsonantZFWJ = 0;
int vQuickStartConsonant = 0;
int vQuickEndConsonant = 0;
int vOtherLanguage = 1;
int vRememberCode = 1;
int vTempOffVietType = 0;

int vUseGrayIcon = 0;

int vSupportMetroApp = 1;
//beta feature
int vFixChromiumBrowser = 0; //new on version 2.0

bool AppDelegate::isDialogMsg(MSG & msg) const {
    return (mainWindow && IsWindowVisible(mainWindow->getHwnd()) &&
            IsDialogMessage(mainWindow->getHwnd(), &msg)) ||
           (macroDialog && IsDialogMessage(macroDialog->getHwnd(), &msg)) ||
           (convertDialog && IsDialogMessage(convertDialog->getHwnd(), &msg));
}

AppDelegate::AppDelegate() {
	_instance = this;
}

AppDelegate * AppDelegate::getInstance() {
	return _instance;
}

int AppDelegate::run(HINSTANCE hInstance) {
	this->hInstance = hInstance;

	//check app has already run or not
	HWND previousInstance = SystemTrayHelper::findRunningWindow();
	if (previousInstance) {
        DWORD_PTR build = 0;
        const bool responds = SendMessageTimeout(previousInstance, queryVietTypeBuild, 0, 0,
                                                  SMTO_ABORTIFHUNG | SMTO_BLOCK, 1000, &build);
        if (responds && build == VIETTYPE_VERSION_NUMBER) {
            PostMessage(previousInstance, showVietTypeWindow, 0, 0);
        } else {
            MessageBox(nullptr,
                L"Một bản VietType khác đang chạy. Nhấp phải vào biểu tượng V/E ở khay hệ thống, "
                L"chọn Thoát, rồi mở lại tệp này để xem cửa sổ đầy đủ.",
                L"VietType", MB_OK | MB_ICONINFORMATION);
        }
		return 0;
	}

    const bool trayReady = SystemTrayHelper::createSystemTrayIcon(hInstance);
    VietTypeManager::refreshImeStatus();
    if (!VietTypeManager::initEngine()) {
        VietTypeManager::freeEngine();
        SystemTrayHelper::removeSystemTray();
        MessageBox(nullptr, L"Không khởi tạo được bộ gõ. VietType chưa nhận phím.",
                   L"VietType", MB_OK | MB_ICONERROR);
        return 1;
    }
    SystemTrayHelper::updateData();

    MainWindow panel(hInstance, *this);
    panel.show(!trayReady || viettype::showPanelAtLaunch() != 0);
    mainWindow = &panel;
    if (trayReady && panel.getHwnd()) viettype::confirmUpdateReady();
    if (!trayReady) {
        MessageBox(panel.getHwnd(), L"Không tạo được biểu tượng khay. Bản dự phòng (nếu có) vẫn được giữ.",
                   L"VietType", MB_OK | MB_ICONWARNING);
    }
    if (viettype::checkUpdatesAtLaunch()) {
        viettype::checkForUpdate(panel.getHwnd());
    }

    MSG msg;
	// Main message loop:
	while (GetMessage(&msg, nullptr, 0, 0) > 0)	{
		if (msg.message == WM_KEYDOWN) {
			VietTypeManager::_lastKeyCode = (UINT16)msg.wParam;
		}
		if (!isDialogMsg(msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	if (macroDialog) closeDialog(macroDialog);
	if (convertDialog) closeDialog(convertDialog);
	mainWindow = nullptr;
	panel.closeDialog();
	VietTypeManager::freeEngine();
	SystemTrayHelper::removeSystemTray();
	return 0;
}

void AppDelegate::closeDialog(BaseDialog * dialog) {
	dialog->closeDialog();
	if (macroDialog == dialog) {
		delete macroDialog;
		macroDialog = NULL;
	} else if (convertDialog == dialog) {
		delete convertDialog;
		convertDialog = NULL;
	}
	refreshMainWindow();
	SystemTrayHelper::updateData();
}

void AppDelegate::onInputMethodChangedFromHotKey() {
	APP_SET_DATA(vLanguage, vLanguage);
	refreshMainWindow();
	SystemTrayHelper::updateData();
}

void AppDelegate::onToggleVietnamese() {
	APP_SET_DATA(vLanguage, vLanguage ? 0 : 1);
	
	if (vUseSmartSwitchKey) {
		string& exe = VietTypeHelper::getLastAppExecuteName();
		setAppInputMethodStatus(exe, vLanguage | (vCodeTable << 1));
		saveSmartSwitchKeyData();
	}
	refreshMainWindow();
	SystemTrayHelper::updateData();
}

void AppDelegate::onMacroTable() {
	if (macroDialog == NULL) {
		macroDialog = new MacroDialog(hInstance, IDD_DIALOG_MACRO);
		macroDialog->show();
	} else {
		macroDialog->bringOnTop();
	}
}

void AppDelegate::onConvertTool() {
	if (convertDialog == NULL) {
		convertDialog = new ConvertToolDialog(hInstance, IDD_DIALOG_CONVERT_TOOL);
		convertDialog->show();
	} else {
		convertDialog->bringOnTop();
	}
}

void AppDelegate::onQuickConvert() {
	if (VietTypeHelper::quickConvert()) {
		//alert when complete
		if (!convertToolDontAlertWhenCompleted) {
			TCHAR msg[256];
			LoadString(hInstance, IDS_STRING_CONVERT_COMPLETED, msg, 256);
			MessageBox(NULL, msg, _T("VietType"), MB_OK);
		}
	}
}

void AppDelegate::onInputType(const int & type) {
    if (type < vTelex || type > vBilingualVniTelex) return;
    if (type != vInputType) {
        APP_SET_DATA(vInputType, type);
        startNewSession();
    }
	refreshMainWindow();
	SystemTrayHelper::updateData();
}

void AppDelegate::onTableCode(const int & code) {
	if (code < 0 || static_cast<size_t>(code) >= VietTypeManager::getTableCode().size()) return;
    if (code != vCodeTable) {
        APP_SET_DATA(vCodeTable, code);
        VietTypeManager::refreshInputEncoding();
        onTableCodeChange();
    }
	if (vRememberCode) {
		setAppInputMethodStatus(VietTypeHelper::getFrontMostAppExecuteName(), vLanguage | (vCodeTable << 1));
		saveSmartSwitchKeyData();
	}
	refreshMainWindow();
	SystemTrayHelper::updateData();
}

void AppDelegate::refreshMainWindow() {
	if (mainWindow) mainWindow->refreshInputControls();
}

void AppDelegate::onControlPanel() {
	if (!mainWindow) return;
	mainWindow->fillData();
	ShowWindow(mainWindow->getHwnd(), SW_RESTORE);
	mainWindow->bringOnTop();
}

void AppDelegate::onVietTypeExit() {
	PostQuitMessage(0);
}
