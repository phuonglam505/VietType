// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <array>
#include <charconv>
#include <optional>
#include <string_view>

namespace viettype {
using ReleaseVersion = std::array<unsigned, 3>;

inline std::optional<ReleaseVersion> parseReleaseVersion(std::string_view tag)
{
    if (!tag.empty() && (tag.front() == 'v' || tag.front() == 'V')) {
        tag.remove_prefix(1);
        if (!tag.empty() && tag.front() == '.') tag.remove_prefix(1);
    }
    ReleaseVersion version{};
    for (size_t index = 0; index < version.size(); ++index) {
        const auto separator = tag.find('.');
        if ((separator == std::string_view::npos) != (index == version.size() - 1)) return {};
        const auto part = tag.substr(0, separator);
        if (part.empty() || (part.size() > 1 && part.front() == '0')) return {};
        const auto end = part.data() + part.size();
        const auto result = std::from_chars(part.data(), end, version[index]);
        if (result.ec != std::errc{} || result.ptr != end) return {};
        if (separator != std::string_view::npos) tag.remove_prefix(separator + 1);
    }
    return version;
}

inline std::optional<ReleaseVersion> parseGitHubReleaseVersion(std::string_view tag,
                                                               std::string_view title)
{
    if (const auto version = parseReleaseVersion(tag)) return version;

    constexpr std::string_view product = "viettype";
    const auto withoutProduct = [product](std::string_view value) {
        if (value.size() < product.size()) return value;
        for (size_t i = 0; i < product.size(); ++i) {
            char ch = value[i];
            if (ch >= 'A' && ch <= 'Z') ch += 'a' - 'A';
            if (ch != product[i]) return value;
        }
        value.remove_prefix(product.size());
        const auto first = value.find_first_not_of(" -_");
        return first == std::string_view::npos ? std::string_view{} : value.substr(first);
    };
    if (const auto version = parseReleaseVersion(withoutProduct(tag))) return version;
    return parseReleaseVersion(withoutProduct(title));
}
}
