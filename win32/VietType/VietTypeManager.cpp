/*----------------------------------------------------------
OpenKey

Copyright (C) 2019 Mai Vu Tuyen

This file is belong to the OpenKey project, Win32 version
which is released under GPL license.
You can fork, modify, improve this program. If you
redistribute your new version, it MUST be open source.
-----------------------------------------------------------*/
#include "VietTypeManager.h"

static vector<LPCTSTR> _inputType = {
	_T("Telex"),
	_T("VNI"),
	_T("Simple Telex"),
};

static vector<LPCTSTR> _tableCode = {
	_T("Unicode"),
	_T("TCVN3 (ABC)"),
	_T("VNI Windows"),
	_T("Unicode Tổ hợp"),
	_T("Windows CP1258")
};

/*-----------------------------------------------------------------------*/

extern void VietTypeInit();
extern void VietTypeFree();

unsigned short  VietTypeManager::_lastKeyCode = 0;

vector<LPCTSTR>& VietTypeManager::getInputType() {
	return _inputType;
}

vector<LPCTSTR>& VietTypeManager::getTableCode() {
	return _tableCode;
}

void VietTypeManager::initEngine() {
	VietTypeInit();
}

void VietTypeManager::freeEngine() {
	VietTypeFree();
}
