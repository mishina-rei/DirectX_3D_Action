#include "Engine_pch.h"

#include "Scene.h"
#include "RenderSystem.h"
#include "ScriptSystem.h"
#include "ColliderSystem.h"
#include "PhysicsSystem.h"

void Scene::Update()
{
	ScriptSystem::Update(&world);
	commandBuffer.Execute(world);

	PhysicsSystem::Update(&world);

	ColliderSystem::Instance().Check(&world);
	commandBuffer.Execute(world);

	ScriptSystem::LateUpdate(&world);
	commandBuffer.Execute(world);
}

void Scene::Draw()
{
    RenderSystem::Draw(&world);
}

void Scene::Uninit()
{
}
