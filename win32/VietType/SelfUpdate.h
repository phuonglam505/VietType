// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "stdafx.h"
#include "ReleaseVersion.h"

namespace viettype {
struct UpdateAsset {
    ReleaseVersion version{};
    std::wstring url;
    std::wstring digest;
};

bool validUpdateAsset(const UpdateAsset& asset);
std::vector<BYTE> downloadUpdate(const UpdateAsset& asset);
bool replaceAndRestart(const UpdateAsset& asset, const std::vector<BYTE>& payload, std::wstring& error);
bool completeUpdate(LPCWSTR arguments);
void confirmUpdateReady();
}
