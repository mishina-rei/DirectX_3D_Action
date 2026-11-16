// ScriptRegistry.h
// スクリプトの名前からインスタンスを生成するためのレジストリクラス
// 型名をキーにして、スクリプトのインスタンスを生成する関数を登録する

#pragma once

#include "NativeScript.h"
#include "json.hpp"

#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

class ScriptRegistry
{
public:
    static ScriptRegistry& Get();

    ScriptRegistry(const ScriptRegistry&) = delete;
    ScriptRegistry& operator=(const ScriptRegistry&) = delete;

	// スクリプトの型名とスクリプトのインスタンスを生成する関数を登録する
	// TはNativeScriptを継承したクラスである必要がある
    template<typename T>
    bool Register(const std::string& typeName)
    {
        static_assert(std::is_base_of_v<NativeScript, T>);
        static_assert(std::is_default_constructible_v<T>);

        if (typeName.empty())
        {
            return false;
        }

        return factories.emplace(
            typeName,
            []() -> std::shared_ptr<NativeScript>
            {
                return std::make_shared<T>();
            }).second;
    }

	// スクリプトの型名からスクリプトのインスタンスを生成する
    std::shared_ptr<NativeScript> Create(const std::string& typeName) const;

	// スクリプトの型名と設定を指定してスクリプトのインスタンスを生成する
    std::shared_ptr<NativeScript> Create(const std::string& typeName, const nlohmann::json& settings) const;

	// 登録されているスクリプトの型名を全部取得する
    std::vector<std::string> GetTypeNames() const;

private:
    ScriptRegistry() = default;

    std::unordered_map<std::string,std::function<std::shared_ptr<NativeScript>()>> factories;
};
