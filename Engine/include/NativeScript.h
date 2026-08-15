/*****************************************************************//**
 * @file   NativeScript.h
 * @brief  Scriptシステム用のユーザースクリプト基底クラス
 *
 * @author
 * @date
 *********************************************************************/

#pragma once

#include "world.h"
#include <functional>
#include "SceneManager.h"

 // ユーザー定義スクリプトの基底クラス
class NativeScript
{
protected:
	ECS::World* world = nullptr;
	ECS::EntityID entityId = 0;
	bool isInitialized = false;

public:
	virtual ~NativeScript() = default;

	// システムが呼ぶ初期化関数
	void Init(ECS::World* _world, ECS::EntityID _id)
	{
		world = _world;
		entityId = _id;
		OnCreate();
		isInitialized = true;
	}

	bool IsInitialized() const { return isInitialized; }

	// ユーザーがオーバーライドする関数
	virtual void OnCreate() {}
	virtual void Update() {}
	virtual void LateUpdate() {}
	virtual void OnDestroy() {}

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