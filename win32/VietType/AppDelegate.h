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
#include "ConvertToolDialog.h"
#include "MacroDialog.h"

class BaseDialog;
class MainWindow;
class AppDelegate {
private:
	HINSTANCE hInstance;
	MainWindow* mainWindow = nullptr;
	void refreshMainWindow();
	BaseDialog* macroDialog = nullptr, *convertDialog = nullptr;
private:
	bool isDialogMsg(MSG &msg) const;

public:
	AppDelegate();

	static AppDelegate* getInstance();
	int run(HINSTANCE hInstance);

	void closeDialog(BaseDialog* dialog);
public: //event
	void onInputMethodChangedFromHotKey();


	void onToggleVietnamese();




	void onMacroTable();
	void onConvertTool();
	void onQuickConvert();

	void onInputType(const int& type);
	void onTableCode(const int& code);

	void onControlPanel();

	void onVietTypeExit();
};