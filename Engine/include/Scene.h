// Scene.h

#pragma once

#include <vector>
#include "world.h"
#include "CommandBuffer.h"

class Scene
{
public:
	virtual ~Scene() = default;

	virtual void Init() {};
	virtual void Update();
	void Draw();
	virtual void Uninit();

	ECS::World* GetWorld() { return &world; }

	// コマンドバッファを取得
	CommandBuffer* GetCommandBuffer() { return &commandBuffer; }

	// 溜まったコマンドを実行（フレームの最後などで呼ぶ）
	void ExecuteCommands() { commandBuffer.Execute(world); }

	// Worldの機能をSceneから直接呼べるようにするラッパー関数
	ECS::EntityID CreateEntity() { return world.CreateEntity(); }

	template <typename T>
	void RegisterStructural() { world.RegisterStructural<T>(); }

	template <typename T>
	void AddComponent(ECS::EntityID id, T data) { world.AddComponent<T>(id, data); }

	template <typename T>
	T& GetComponent(ECS::EntityID id) { return world.GetComponent<T>(id); }

protected:

	ECS::World world;
	CommandBuffer commandBuffer;
};
