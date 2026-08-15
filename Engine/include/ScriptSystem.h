#pragma once
#include "world.h"

class ScriptSystem
{
public:

	// スクリプトの初期化処理を実行
	static void Init(ECS::World* world);

	// スクリプトの更新処理を実行
	static void Update(ECS::World* world);

	// スクリプトの後更新処理を実行
	static void LateUpdate(ECS::World* world);
};