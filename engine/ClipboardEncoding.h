// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <optional>
#include <string>
#include <string_view>
#include <vector>

std::vector<const wchar_t*> clipboardEncodingNames();
std::optional<std::wstring> convertClipboardEncoding(std::wstring_view text, size_t from, size_t to);

std::optional<std::wstring> encodeTypingCharacter(wchar_t letter, size_t index);
