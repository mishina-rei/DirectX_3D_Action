#include "Engine_pch.h"
#include "ScriptSystem.h"
#include "Script.h"


void ScriptSystem::Init(ECS::World* world)
{
	// ScriptComponentを持つすべてのエンティティを走査
	world->ForEachComponent<Script>([world](ECS::EntityID id, Script& scriptComp) {
		if (!scriptComp.script) return;

		// 未初期化の場合は初期化処理を行う (OnCreate呼び出し)
		if (!scriptComp.script->IsInitialized())
		{
			scriptComp.script->Init(world, id);
		}
	});
}

void ScriptSystem::Update(ECS::World* world)
{
	// ScriptComponentを持つすべてのエンティティを走査
	world->ForEachComponent<Script>([world](ECS::EntityID id, Script& scriptComp) {
		if (!scriptComp.script) return;
		
		// 更新処理
		scriptComp.script->Update();
	});
}

void ScriptSystem::LateUpdate(ECS::World* world)
{
	// ScriptComponentを持つすべてのエンティティを走査
	world->ForEachComponent<Script>([world](ECS::EntityID id, Script& scriptComp) {
		if (!scriptComp.script) return;

		// 更新処理
		scriptComp.script->LateUpdate();
		});
}
