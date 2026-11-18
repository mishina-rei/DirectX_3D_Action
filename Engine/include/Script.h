#pragma once
#include <memory>
#include "NativeScript.h"
#include "json.hpp"
#include <string>

class ImGuiArchive;
class ResolveArchive;

// スクリプトを保持するコンポーネント
struct Script
{
	std::shared_ptr<NativeScript> script;

	bool Bind(const std::string& newTypeName);

	bool HasUnavailableType() const
	{
		return typeName.empty() ? script != nullptr : script == nullptr;	// 型名が空ならスクリプトが存在してはいけない、型名が空でなければスクリプトが存在しなければならない
	}

	// property()で対応できないため、個別に処理を書く
	void Reflect(JsonWriteArchive& archive);
	void Reflect(JsonReadArchive& archive);
#if WITH_EDITOR
	void Reflect(ImGuiArchive& archive);
#endif
	void Reflect(ResolveArchive& archive);

private:
	std::string typeName;
	nlohmann::json savedSettings = nlohmann::json::object();
};
