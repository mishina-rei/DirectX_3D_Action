#pragma once
#include <memory>
#include "NativeScript.h"

// スクリプトを保持するコンポーネント
struct Script
{
	std::shared_ptr<NativeScript> script;

	// スクリプトを生成してバインドするヘルパー
	template <typename T, typename... Args>
	void Bind(Args&&... args)
	{
		script = std::make_shared<T>(std::forward<Args>(args)...);
	}
};