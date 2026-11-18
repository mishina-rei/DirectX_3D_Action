/*****************************************************************//**
 * @file   NativeScript.h
 * @brief  Scriptシステム用のユーザースクリプト基底クラス
 *
 * @author
 * @date
 *********************************************************************/

#pragma once

#include "world.h"
#include "EngineConfig.h"
#include <functional>
#include "SceneManager.h"

class JsonWriteArchive;
class JsonReadArchive;
class ImGuiArchive;

 // ユーザー定義スクリプトの基底クラス
class NativeScript
{
protected:
	ECS::World* world = nullptr;
	ECS::EntityID entityId = 0;
	bool isInitialized = false;

public:
	virtual ~NativeScript() = default;

	// JSON形式でスクリプトの設定を保存・読み込みするための関数
	// 
	// 例　↓
	// 
	//void SaveSettings(JsonWriteArchive& archive) override
	//{
	//	archive.Property("MoveSpeed", moveSpeed);
	//}

	//void LoadSettings(JsonReadArchive& archive) override
	//{
	//	archive.Property("MoveSpeed", moveSpeed);
	//}
	virtual void SaveSettings(JsonWriteArchive& archive) {}
	virtual void LoadSettings(JsonReadArchive& archive) {}
#if WITH_EDITOR
	// 設定UIはエディタでのみ使用する
	virtual void DrawSettings(ImGuiArchive& archive) {}
#endif

	// システムが呼ぶ初期化関数
	void Init(ECS::World* _world, ECS::EntityID _id)
	{
		world = _world;
		entityId = _id;
		OnCreate();
		isInitialized = true;
	}

	bool IsInitialized() const { return isInitialized; }

	void Destroy()	// エンティティが破棄されるときに呼ばれる
	{
		if (!isInitialized)
		{
			return;
		}

		isInitialized = false;
		OnDestroy();

		world = nullptr;
		entityId = ECS::INVALID_ENTITY_ID;
	}

	// ユーザーがオーバーライドする関数
	virtual void OnCreate() {}	// エンティティが生成されたときに呼ばれる
	virtual void Update() {}	// 毎フレーム呼ばれる
	virtual void LateUpdate() {}// 毎フレーム呼ばれる（Updateの後）
	virtual void OnDestroy() {}	// エンティティが破棄されるときに呼ばれる

	// 当たり判定イベント
	virtual void OnCollisionEnter(ECS::EntityID other) {}
	virtual void OnCollisionStay(ECS::EntityID other) {}
	virtual void OnCollisionExit(ECS::EntityID other) {}

protected:

	// コンポーネント取得ヘルパー
	template <typename T>
	T& GetComponent()
	{
		return world->GetComponent<T>(entityId);
	}

	template <typename T>
	bool HasComponent()
	{
		return world->HasComponent<T>(entityId);
	}

	// コマンド（ラムダ式など）を積む
	void Push(std::function<void(ECS::World&)> cmd) {
		SceneManager::GetInstance().GetCurrentScene()->GetCommandBuffer()->Push(cmd);
	}
};
