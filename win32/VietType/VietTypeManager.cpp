/*----------------------------------------------------------
OpenKey

Copyright (C) 2019 Mai Vu Tuyen

This file is belong to the OpenKey project, Win32 version
which is released under GPL license.
You can fork, modify, improve this program. If you
redistribute your new version, it MUST be open source.
-----------------------------------------------------------*/
#include "VietTypeManager.h"
#include "../../engine/ClipboardEncoding.h"

static const vector<VietTypeManager::InputMethod> _inputType = {
    {vVNI, L"VNI"},
    {vBilingualTelex, L"Telex - giữ từ tiếng Anh"},
    {vTelex, L"Telex"},
    {vSimpleTelex1, L"Simple Telex"},
    {vVniTelex, L"VNI + Telex"},
    {vBilingualVniTelex, L"VNI + Telex - giữ từ tiếng Anh"},
};

static auto _tableCode = clipboardEncodingNames();

/*-----------------------------------------------------------------------*/

extern bool VietTypeInit();
extern void VietTypeFree();

unsigned short  VietTypeManager::_lastKeyCode = 0;

const vector<VietTypeManager::InputMethod>& VietTypeManager::getInputType() {
	return _inputType;
}

vector<LPCTSTR>& VietTypeManager::getTableCode() {
	return _tableCode;
}

bool VietTypeManager::initEngine() {
	return VietTypeInit();
}

void VietTypeManager::freeEngine() {
	VietTypeFree();
}
