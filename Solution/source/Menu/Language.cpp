/*
* Menyoo PC - Grand Theft Auto V single-player trainer mod
* Copyright (C) 2019  MAFINS
*/
#include "Language.h"

#include "..\Util\ExePath.h"
#include "..\Util\FileLogger.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <json\single_include\nlohmann\json.hpp>
#include <limits>
#include <map>
#include <set>

using Json = nlohmann::json;

namespace Language
{
namespace
{
struct Entry
{
    bool plural = false;
    std::string text;
    std::string counter;
    std::map<std::string, std::string> forms;
};

using Catalog = std::map<std::string, Entry>;

std::map<std::string, Catalog> catalogs;
std::string selectedLocale = "en-US";
std::set<std::string> reportedDiagnostics;

bool IsLocaleTag(const std::string& tag)
{
    if (tag.size() < 2)
        return false;

    bool afterScript = false;
    bool afterRegion = false;
    size_t segmentStart = 0;
    int segment = 0;
    for (size_t index = 0; index <= tag.size(); ++index)
    {
        if (index != tag.size() && tag[index] != '-')
            continue;

        const size_t length = index - segmentStart;
        if (length == 0)
            return false;
        for (size_t character = segmentStart; character < index; ++character)
        {
            if (!std::isalnum(static_cast<unsigned char>(tag[character])))
                return false;
        }

        if (segment == 0 && (length < 2 || length > 3))
            return false;
        if (segment > 0 && length != 2 && length != 3 && length != 4)
            return false;
        if (length == 4)
            afterScript = true;
        if ((length == 2 || length == 3) && segment > 0)
            afterRegion = true;
        if (afterRegion && length == 4)
            return false;

        ++segment;
        segmentStart = index + 1;
    }
    return !afterScript || segment > 1;
}

std::string NormalizeLocale(std::string tag)
{
    for (char& character : tag)
        character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));

    size_t separator = tag.find('-');
    if (separator != std::string::npos)
    {
        const size_t nextSeparator = tag.find('-', separator + 1);
        const size_t secondLength = (nextSeparator == std::string::npos ? tag.size() : nextSeparator) - separator - 1;
        if (secondLength == 4)
        {
            for (size_t index = separator + 1; index < separator + 1 + secondLength; ++index)
                tag[index] = static_cast<char>(std::toupper(static_cast<unsigned char>(tag[index])));
            separator = nextSeparator;
        }
        if (separator != std::string::npos)
        {
            for (size_t index = separator + 1; index < tag.size(); ++index)
                tag[index] = static_cast<char>(std::toupper(static_cast<unsigned char>(tag[index])));
        }
    }
    return tag;
}

void ReportOnce(const std::string& message)
{
    if (reportedDiagnostics.insert(message).second)
        addlog(ige::LogType::LOG_ERROR, message);
}

bool ReadString(const Json& value, std::string& result)
{
    if (!value.is_string())
        return false;
    result = value.get<std::string>();
    return !result.empty();
}

bool ReadEntry(const std::string& key, const Json& value, Entry& entry)
{
    if (value.is_string())
    {
        entry.text = value.get<std::string>();
        return !entry.text.empty();
    }
    if (!value.is_object() || !value.contains("plural") || !value.contains("forms") || !value["plural"].is_string() || !value["forms"].is_object())
        return false;

    entry.plural = true;
    entry.counter = value["plural"].get<std::string>();
    if (entry.counter.empty())
        return false;
    for (const auto& [category, form] : value["forms"].items())
    {
        std::string formText;
        if (!ReadString(form, formText))
            return false;
        entry.forms.emplace(category, std::move(formText));
    }
    return entry.forms.find("other") != entry.forms.end();
}

bool LoadCatalog(const std::string& path, Catalog& result, std::string& displayName)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
        return false;

    try
    {
        Json document = Json::parse(stream);
        if (!document.is_object())
            return false;

        displayName.clear();
        if (document.contains("_meta") && document["_meta"].is_object() && document["_meta"].contains("displayName") && document["_meta"]["displayName"].is_string())
            displayName = document["_meta"]["displayName"].get<std::string>();

        for (const auto& [key, value] : document.items())
        {
            if (key == "_meta")
                continue;
            Entry entry;
            if (!ReadEntry(key, value, entry))
            {
                ReportOnce("Invalid localization entry: " + key + " in " + path);
                continue;
            }
            result[key] = std::move(entry);
        }
    }
    catch (const std::exception& exception)
    {
        ReportOnce("Unable to load language file " + path + ": " + exception.what());
        return false;
    }
    return true;
}

std::string PluralCategory(const std::string& locale, std::uint64_t value)
{
    const std::string language = locale.substr(0, locale.find('-'));
    if (language == "ja" || language == "ko" || language == "zh" || language == "th" || language == "vi")
        return "other";
    if (language == "fr")
        return value <= 1 ? "one" : "other";
    if (language == "pt")
        return value <= 1 || (locale == "pt-BR" && value == 0) ? "one" : "other";
    if (language == "ru" || language == "uk")
    {
        if (value % 10 == 1 && value % 100 != 11)
            return "one";
        if (value % 10 >= 2 && value % 10 <= 4 && (value % 100 < 12 || value % 100 > 14))
            return "few";
        return "many";
    }
    if (language == "pl")
    {
        if (value == 1)
            return "one";
        if (value % 10 >= 2 && value % 10 <= 4 && (value % 100 < 12 || value % 100 > 14))
            return "few";
        return "many";
    }
    if (language == "cs" || language == "sk")
    {
        if (value == 1)
            return "one";
        if (value >= 2 && value <= 4)
            return "few";
        return "other";
    }
    if (language == "ar")
    {
        if (value == 0)
            return "zero";
        if (value == 1)
            return "one";
        if (value == 2)
            return "two";
        if (value % 100 >= 3 && value % 100 <= 10)
            return "few";
        if (value % 100 >= 11 && value % 100 <= 99)
            return "many";
        return "other";
    }
    return value == 1 ? "one" : "other";
}

bool GetInteger(const ArgumentValue& value, std::uint64_t& magnitude)
{
    if (const auto* unsignedValue = std::get_if<std::uint64_t>(&value))
    {
        magnitude = *unsignedValue;
        return true;
    }
    const auto* signedValue = std::get_if<std::int64_t>(&value);
    if (signedValue == nullptr)
        return false;
    if (*signedValue < 0)
        magnitude = static_cast<std::uint64_t>(-(*signedValue + 1)) + 1;
    else
        magnitude = static_cast<std::uint64_t>(*signedValue);
    return true;
}

const Entry* FindEntry(const std::string& locale, const std::string& key)
{
    const auto catalog = catalogs.find(locale);
    if (catalog == catalogs.end())
        return nullptr;
    const auto entry = catalog->second.find(key);
    return entry == catalog->second.end() ? nullptr : &entry->second;
}

bool FormatTemplate(const std::string& input, const std::map<std::string, ArgumentValue>& arguments, std::string& output)
{
    output.clear();
    for (size_t index = 0; index < input.size();)
    {
        if (input[index] == '{')
        {
            if (index + 1 < input.size() && input[index + 1] == '{')
            {
                output += '{';
                index += 2;
                continue;
            }
            const size_t end = input.find('}', index + 1);
            if (end == std::string::npos || end == index + 1)
                return false;
            const std::string name = input.substr(index + 1, end - index - 1);
            const auto argument = arguments.find(name);
            if (argument == arguments.end() || name.find('{') != std::string::npos)
                return false;
            std::visit(
                [&output](const auto& value)
                {
                    using ValueType = std::decay_t<decltype(value)>;
                    if constexpr (std::is_same_v<ValueType, std::string>)
                        output += value;
                    else
                        output += std::to_string(value);
                },
                argument->second
            );
            index = end + 1;
            continue;
        }
        if (input[index] == '}' && index + 1 < input.size() && input[index + 1] == '}')
        {
            output += '}';
            index += 2;
            continue;
        }
        if (input[index] == '}')
            return false;
        output += input[index++];
    }
    return true;
}

bool FormatEntry(const Entry& entry, const std::string& locale, const std::map<std::string, ArgumentValue>& arguments, std::string& output)
{
    if (!entry.plural)
        return FormatTemplate(entry.text, arguments, output);

    const auto counter = arguments.find(entry.counter);
    if (counter == arguments.end())
        return false;
    std::uint64_t magnitude = 0;
    if (!GetInteger(counter->second, magnitude))
        return false;

    std::string selector = "=" + std::to_string(magnitude);
    auto form = entry.forms.find(selector);
    if (form == entry.forms.end())
        form = entry.forms.find(PluralCategory(locale, magnitude));
    if (form == entry.forms.end())
        form = entry.forms.find("other");
    return form != entry.forms.end() && FormatTemplate(form->second, arguments, output);
}

} // namespace

std::vector<LocaleInfo> allLangs;
std::string configLangName;

Argument::Argument(std::string argumentName, std::string argumentValue) : name(std::move(argumentName)), value(std::move(argumentValue))
{
}

Argument::Argument(std::string argumentName, const char* argumentValue) : name(std::move(argumentName)), value(std::string(argumentValue == nullptr ? "" : argumentValue))
{
}

Argument::Argument(std::string argumentName, std::int64_t argumentValue) : name(std::move(argumentName)), value(argumentValue)
{
}

Argument::Argument(std::string argumentName, std::uint64_t argumentValue) : name(std::move(argumentName)), value(argumentValue)
{
}

std::string Format(std::string_view keyView, std::initializer_list<Argument> arguments)
{
    const std::string key(keyView);
    std::map<std::string, ArgumentValue> values;
    for (const Argument& argument : arguments)
    {
        if (!values.emplace(argument.name, argument.value).second)
        {
            ReportOnce("Duplicate localization argument: " + argument.name);
            return key;
        }
    }

    const Entry* entry = FindEntry(selectedLocale, key);
    std::string result;
    if (entry != nullptr && FormatEntry(*entry, selectedLocale, values, result))
        return result;

    const Entry* englishEntry = FindEntry("en-US", key);
    if (englishEntry != nullptr && FormatEntry(*englishEntry, "en-US", values, result))
        return result;

    ReportOnce("Missing or invalid localization key: " + key);
    return key;
}

std::string Translate(std::string_view key)
{
    return Format(key, {});
}

std::string TranslateToSelected(std::string text)
{
    if (FindEntry(selectedLocale, text) != nullptr || FindEntry("en-US", text) != nullptr)
        return Translate(text);
    return text;
}

std::string TranslateStable(std::string_view prefix, std::string_view identifier, std::string_view fallback)
{
    std::string key(prefix);
    key.push_back('.');
    for (const unsigned char character : identifier)
    {
        if (std::isalnum(character))
            key.push_back(static_cast<char>(std::tolower(character)));
        else
            key.push_back('_');
    }

    const std::string translated = TranslateToSelected(key);
    return translated == key ? std::string(fallback) : translated;
}

int Init()
{
    const std::string directory = GetPathffA(Pathff::Language, false);
    std::vector<std::string> files;
    get_all_filenames_with_extension(directory, ".json", files, false);

    std::map<std::string, Catalog> loadedCatalogs;
    std::vector<LocaleInfo> loadedLocales;
    for (const std::string& file : files)
    {
        const std::string locale = NormalizeLocale(file);
        if (!IsLocaleTag(locale))
            continue;
        Catalog catalog;
        std::string displayName;
        if (!LoadCatalog(directory + "\\" + file + ".json", catalog, displayName))
            continue;
        loadedCatalogs.emplace(locale, std::move(catalog));
        loadedLocales.push_back({locale, displayName.empty() ? locale : displayName});
    }

    if (loadedCatalogs.find("en-US") == loadedCatalogs.end())
    {
        ReportOnce("Missing required en-US localization catalog");
        catalogs.clear();
        allLangs.clear();
        selectedLocale = "en-US";
        configLangName.clear();
        return -1;
    }

    catalogs = std::move(loadedCatalogs);
    allLangs = std::move(loadedLocales);
    std::sort(
        allLangs.begin(),
        allLangs.end(),
        [](const LocaleInfo& left, const LocaleInfo& right)
        {
            if (left.localeTag == "en-US")
                return true;
            if (right.localeTag == "en-US")
                return false;
            return left.localeTag < right.localeTag;
        }
    );

    if (catalogs.find(NormalizeLocale(configLangName)) == catalogs.end())
        selectedLocale = "en-US";
    else
        selectedLocale = NormalizeLocale(configLangName);
    configLangName = selectedLocale == "en-US" ? std::string() : selectedLocale;
    return 0;
}

void Reload()
{
    Init();
}

int SetSelectedLangFromConfig()
{
    const std::string requested = NormalizeLocale(configLangName);
    if (requested.empty() || requested == "en-US")
    {
        ResetSelectedLang();
        return 0;
    }
    return SetSelectedLangFromString(requested);
}

int SetSelectedLangFromString(std::string localeTag)
{
    localeTag = NormalizeLocale(std::move(localeTag));
    if (localeTag.empty() || localeTag == "en-US")
    {
        ResetSelectedLang();
        return 0;
    }
    if (catalogs.find(localeTag) == catalogs.end())
        return -1;
    selectedLocale = localeTag;
    configLangName = localeTag;
    return 0;
}

void ResetSelectedLang()
{
    selectedLocale = "en-US";
    configLangName.clear();
}

std::string GetSelectedLangTitle()
{
    return GetLocaleTitle(selectedLocale);
}

std::string GetSelectedLocale()
{
    return selectedLocale;
}

std::string GetLocaleTitle(std::string_view localeTag)
{
    const std::string normalizedLocale = NormalizeLocale(std::string(localeTag));
    for (const LocaleInfo& locale : allLangs)
    {
        if (locale.localeTag == normalizedLocale)
            return locale.displayName;
    }
    return normalizedLocale;
}

const std::vector<LocaleInfo>& GetAvailableLocales()
{
    return allLangs;
}

} // namespace Language
