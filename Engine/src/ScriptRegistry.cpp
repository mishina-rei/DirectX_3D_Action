#include "Engine_pch.h"
#include "ScriptRegistry.h"
#include "Archive.h"

#include <algorithm>

ScriptRegistry& ScriptRegistry::Get()
{
    static ScriptRegistry instance;
    return instance;
}

std::shared_ptr<NativeScript> ScriptRegistry::Create(const std::string& typeName) const
{
	const auto entry = factories.find(typeName);    // factoriesから検索

    if (entry == factories.end())
    {
        return nullptr;
    }

    return entry->second();
}

std::shared_ptr<NativeScript> ScriptRegistry::Create(const std::string& typeName,const nlohmann::json& settings) const
{
    if (!settings.is_object())
    {
        return nullptr;
    }

	auto script = Create(typeName); // 型名からスクリプトのインスタンスを生成する

    if (!script)
    {
        return nullptr;
    }

    try
    {
        JsonReadArchive archive(settings);
		script->LoadSettings(archive);  // スクリプトの設定をJSONから読み込む
    }
    catch (const nlohmann::json::exception&)
    {
        return nullptr;
    }

    return script;
}

std::vector<std::string> ScriptRegistry::GetTypeNames() const
{
    std::vector<std::string> typeNames;
    typeNames.reserve(factories.size());

    for (const auto& entry : factories)
    {
		typeNames.push_back(entry.first);   // factoriesのキー（スクリプトの型名）をtypeNamesに追加
    }

    std::sort(typeNames.begin(), typeNames.end());

    return typeNames;
}
