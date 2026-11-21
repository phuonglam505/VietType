// SPDX-License-Identifier: GPL-3.0-or-later
#include "ClipboardEncoding.h"
#include "ConvertTool.h"
#include "vnconv.h"
#include <array>
#include <cstring>
#include <limits>

namespace {
struct Encoding { const wchar_t* name; int id; bool wide; };
// The first five indexes are persisted by earlier versions of VietType.
constexpr std::array<Encoding, 17> encodings{{
    {L"Unicode", CONV_CHARSET_UNICODE, true},
    {L"TCVN3 (ABC)", CONV_CHARSET_TCVN3, false},
    {L"VNI Windows", CONV_CHARSET_VNIWIN, false},
    {L"Unicode tổ hợp", CONV_CHARSET_UNIDECOMPOSED, true},
    {L"Windows CP1258", CONV_CHARSET_WINCP1258, false},
    {L"VIQR", CONV_CHARSET_VIQR, false},
    {L"UTF-8 Literal", CONV_CHARSET_UNIUTF8, false},
    {L"NCR Decimal", CONV_CHARSET_UNIREF, false},
    {L"NCR Hex", CONV_CHARSET_UNIREF_HEX, false},
    {L"VISCII", CONV_CHARSET_VISCII, false},
    {L"VPS", CONV_CHARSET_VPS, false},
    {L"BK HCM 2", CONV_CHARSET_BKHCM2, false},
    {L"BK HCM 1", CONV_CHARSET_BKHCM1, false},
    {L"Vietware X", CONV_CHARSET_VIETWAREX, false},
    {L"Vietware F", CONV_CHARSET_VIETWAREF, false},
    {L"X UTF-8", CONV_CHARSET_XUTF8, false},
    {L"Unicode C String", CONV_CHARSET_UNI_CSTRING, false},
    // ISC is the VNU code page in the VnConv tables.
}};
constexpr Encoding vnu{L"VNU", CONV_CHARSET_ISC, false};

constexpr bool validIndex(size_t index) {
    return index <= encodings.size();
}

const Encoding& encoding(size_t index) {
    return index == encodings.size() ? vnu : encodings[index];
}

std::optional<std::vector<UKBYTE>> convert(std::vector<UKBYTE> input, int from, int to) {
    constexpr auto maxInput = std::numeric_limits<int>::max() / 16 - 8;
    if (input.size() > maxInput) return {};
    if (input.empty()) return std::vector<UKBYTE>{};
    std::vector<UKBYTE> output(input.size() * 8 + 16);
    int remaining = static_cast<int>(input.size());
    int length = static_cast<int>(output.size());
    const auto result = VnConvert(from, to, input.data(), output.data(), &remaining, &length);
    if (result != VNCONV_NO_ERROR || remaining != 0 || length < 0 ||
        static_cast<size_t>(length) > output.size()) return {};
    output.resize(length);
    return output;
}

std::vector<UKBYTE> utf16Bytes(std::wstring_view text) {
    const auto first = reinterpret_cast<const UKBYTE*>(text.data());
    if (text.empty()) return {};
    return {first, first + text.size()*sizeof(wchar_t)};
}

std::optional<std::wstring> widen(const std::vector<UKBYTE>& bytes, bool wide) {
    if (!wide) return std::wstring(bytes.begin(), bytes.end());
    if (bytes.size() % sizeof(wchar_t)) return {};
    std::wstring text(bytes.size()/sizeof(wchar_t), L'\0');
    if (!text.empty()) std::memcpy(text.data(), bytes.data(), bytes.size());
    return text;
}
}

std::vector<const wchar_t*> clipboardEncodingNames() {
    std::vector<const wchar_t*> names;
    for (const auto& item : encodings) names.push_back(item.name);
    names.push_back(vnu.name);
    return names;
}

std::optional<std::wstring> convertClipboardEncoding(std::wstring_view text, size_t from, size_t to) {
    static_assert(sizeof(wchar_t) == 2, "VnConv uses UTF-16 code units");
    if (!validIndex(from) || !validIndex(to)) return {};

    const auto& source = encoding(from);
    const auto& destination = encoding(to);
    std::vector<UKBYTE> input;
    if (source.wide) {
        input = utf16Bytes(text);
    } else {
        input.reserve(text.size());
        for (const wchar_t letter : text) {
            if (letter > 255) return {};
            input.push_back(static_cast<UKBYTE>(letter));
        }
    }

    VnConvOptions options{};
    VnConvResetOptions(&options);
    // Clipboard text may contain standalone syllables and arbitrary snippets.
    options.smartViqr = 0;
    VnConvSetOptions(&options);
    if (convertToolToCapsFirstLetter || convertToolToCapsEachWord) {
        auto decoded = convert(std::move(input), source.id, CONV_CHARSET_UNICODE);
        if (!decoded) return {};
        auto unicode = widen(*decoded, true);
        if (!unicode) return {};
        bool capitalize = true;
        for (auto& letter : *unicode) {
            if (iswspace(letter)) {
                if (convertToolToCapsEachWord || letter == L'\n') capitalize = true;
                continue;
            }
            if (capitalize && iswalpha(letter)) {
                // Windows invariant casing includes Vietnamese Unicode letters.
                CharUpperBuffW(&letter, 1);
                capitalize = false;
            }
            if (letter == L'.' || letter == L'!' || letter == L'?') capitalize = true;
        }
        input = utf16Bytes(*unicode);
    }
    options.toUpper = convertToolToAllCaps;
    options.toLower = convertToolToAllNonCaps;
    options.removeTone = convertToolRemoveMark;
    VnConvSetOptions(&options);
    const int inputId = convertToolToCapsFirstLetter || convertToolToCapsEachWord
        ? CONV_CHARSET_UNICODE : source.id;
    auto output = convert(std::move(input), inputId, destination.id);
    if (!output) return {};
    auto result = widen(*output, destination.wide);
    if (result && result->find(L'\0') != std::wstring::npos) return {};
    return result;
}

std::optional<std::wstring> encodeTypingCharacter(wchar_t letter, size_t index) {
    if (!validIndex(index)) return {};
    VnConvOptions options{};
    VnConvResetOptions(&options);
    options.smartViqr = 0;
    VnConvSetOptions(&options);
    const auto& destination = encoding(index);
    const auto output = convert(utf16Bytes({&letter, 1}), CONV_CHARSET_UNICODE, destination.id);
    return output ? widen(*output, destination.wide) : std::nullopt;
}
