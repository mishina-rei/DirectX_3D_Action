#include "Engine_pch.h"
#include "Script.h"
#include "ScriptRegistry.h"
#include "Archive.h"

#include <utility>

bool Script::Bind(const std::string& newTypeName)
{
    if (script && script->IsInitialized())
    {
		return false;   // 既に初期化済みのスクリプトがある場合はバインドできない
    }

	auto newScript = ScriptRegistry::Get().Create(newTypeName); // 型名からスクリプトのインスタンスを生成する

    if (!newScript)
    {
        return false;
    }

    // 初期化する
    script = std::move(newScript);
    typeName = newTypeName;
	savedSettings = nlohmann::json::object();

    return true;
}

void Script::Reflect(JsonWriteArchive& archive)
{
    nlohmann::json settings = savedSettings;

    if (script)
    {
        JsonWriteArchive settingsArchive;
        settingsArchive.j = nlohmann::json::object();

        script->SaveSettings(settingsArchive);
        settings = std::move(settingsArchive.j);
    }

    archive.j["Type"] = typeName;
    archive.j["Settings"] = std::move(settings);
}

void Script::Reflect(JsonReadArchive& archive)
{
    if (script && script->IsInitialized())
    {
		return; // 既に初期化済みのスクリプトがある場合は読み込みを行わない
    }

    std::string loadedTypeName;
    nlohmann::json loadedSettings = nlohmann::json::object();

	// JSONから型名と設定を読み込む
    archive.Property("Type", loadedTypeName);
    archive.Property("Settings", loadedSettings);

    std::shared_ptr<NativeScript> loadedScript;

    if (!loadedTypeName.empty())
    {
		loadedScript = ScriptRegistry::Get().Create(loadedTypeName, loadedSettings);    // 型名と設定からスクリプトのインスタンスを生成する
    }

    typeName = std::move(loadedTypeName);
    savedSettings = std::move(loadedSettings);
    script = std::move(loadedScript);
}

void Script::Reflect(ImGuiArchive& archive)
{
    ImGui::PushID("Script");

	const bool canChangeType = !script || !script->IsInitialized(); // スクリプトが初期化済みの場合は型の変更を禁止する

    ImGui::BeginDisabled(!canChangeType);

    const char* preview = typeName.empty() ? "(none)" : typeName.c_str();

    if (ImGui::BeginCombo("Type", preview))
    {
        const auto typeNames = ScriptRegistry::Get().GetTypeNames();

		for (const auto& registeredTypeName : typeNames)    // 登録されているスクリプトの型名を列挙する
        {
            const bool isSelected =
                typeName == registeredTypeName;

            if (ImGui::Selectable(registeredTypeName.c_str(), isSelected))
            {
                if (!isSelected || !script)
                {
					Bind(registeredTypeName);   // 型名を変更してスクリプトをバインドする
                }
            }

            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    ImGui::EndDisabled();

    if (script)
    {
		script->DrawSettings(archive); // スクリプトの設定UIを描画する
    }
    else if (!typeName.empty())
    {
        ImGui::TextUnformatted("Script unavailable.");
    }

    ImGui::PopID();
}

void Script::Reflect(ResolveArchive& archive)
{
}
