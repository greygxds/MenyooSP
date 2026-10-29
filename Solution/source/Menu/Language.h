/*
* Menyoo PC - Grand Theft Auto V single-player trainer mod
* Copyright (C) 2019  MAFINS
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*/
#pragma once

#include <cstdint>
#include <initializer_list>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace Language
{
using ArgumentValue = std::variant<std::string, std::int64_t, std::uint64_t>;

struct Argument
{
    std::string name;
    ArgumentValue value;

    Argument(std::string argumentName, std::string argumentValue);
    Argument(std::string argumentName, const char* argumentValue);
    Argument(std::string argumentName, std::int64_t argumentValue);
    Argument(std::string argumentName, std::uint64_t argumentValue);

    template <
        typename T,
        typename = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool> && !std::is_same_v<T, char> && !std::is_same_v<T, signed char> && !std::is_same_v<T, unsigned char>>>
    Argument(std::string argumentName, T argumentValue) : name(std::move(argumentName))
    {
        if constexpr (std::is_signed_v<T>)
            value = static_cast<std::int64_t>(argumentValue);
        else
            value = static_cast<std::uint64_t>(argumentValue);
    }
};

struct LocaleInfo
{
    std::string localeTag;
    std::string displayName;
};

extern std::vector<LocaleInfo> allLangs;
extern std::string configLangName;

std::string Translate(std::string_view key);
std::string Format(std::string_view key, std::initializer_list<Argument> arguments);

std::string TranslateToSelected(std::string text);
std::string TranslateStable(std::string_view prefix, std::string_view identifier, std::string_view fallback);

int Init();
void Reload();
int SetSelectedLangFromConfig();
int SetSelectedLangFromString(std::string localeTag);
void ResetSelectedLang();
std::string GetSelectedLangTitle();
std::string GetSelectedLocale();
std::string GetLocaleTitle(std::string_view localeTag);
const std::vector<LocaleInfo>& GetAvailableLocales();

} // namespace Language
