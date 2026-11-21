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

class VietTypeManager {
public:
	static unsigned short _lastKeyCode;
	struct InputMethod {
        vKeyInputType id;
        LPCTSTR label;
    };
    static const vector<InputMethod>& getInputType();
	static vector<LPCTSTR>& getTableCode();

	static bool initEngine();
	static void refreshImeStatus();
	static void refreshInputEncoding();
	static void freeEngine();


};