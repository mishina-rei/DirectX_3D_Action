#include "Engine_pch.h"

#include "Scene.h"
#include "RenderSystem.h"
#include "ScriptSystem.h"
#include "ColliderSystem.h"
#include "PhysicsSystem.h"
#include "EngineTime.h"

void Scene::Update()
{
	commandBuffer.Execute(world, true);

	ScriptSystem::Update(&world);
	commandBuffer.Execute(world, true);

	PhysicsSystem::Update(&world, Time::GetDeltaTime());

	ColliderSystem::Instance().Check(&world);
	commandBuffer.Execute(world, true);

	ScriptSystem::LateUpdate(&world);
	commandBuffer.Execute(world, true);
}

void Scene::Draw()
{
    RenderSystem::Draw(&world);
}

void Scene::Uninit()
{
	commandBuffer.Clear();
	world.Clear();
	commandBuffer.Clear();

	ColliderSystem::Instance().ClearContactHistory();
}
