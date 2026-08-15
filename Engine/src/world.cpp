#include "Engine_pch.h"
#include "world.h"

ECS::World::World() 
{
    // 空のアーキタイプを作成
    Signature emptySig;
    emptyArchetype = std::make_shared<Archetype>(emptySig);
    archetypeMap[emptySig] = emptyArchetype;
}

ECS::EntityID ECS::World::CreateEntity() 
{
    EntityID id = entityIdGen.Generate();
    size_t idx = emptyArchetype->AddEntity(id);
    entityIndex[id] = { emptyArchetype, idx };
    return id;
}

void ECS::World::DeleteEntity(EntityID id)
{
    // 存在確認
    if (entityIndex.find(id) == entityIndex.end())
        return;

    EntityRecord& record = entityIndex[id];
    auto arch = record.archetype;
    size_t index = record.index;

    // アーキタイプから削除 (Swap & Pop)
    // 戻り値: {移動が発生したか, 移動したEntityID}
    auto result = arch->RemoveEntity(index);

    // Swap削除で別のエンティティが移動してきた場合、そのエンティティのインデックス情報を更新
    if (result.first) {
        entityIndex[result.second].index = index;
    }

    // 管理リストから削除
    entityIndex.erase(id);

    // スパースプール（構造的でないコンポーネント）からも削除
    for (auto& pair : sparsePools) {
        pair.second->Remove(id);
    }

    // IDを解放
    entityIdGen.Release(id);
}

std::shared_ptr<ECS::Archetype> ECS::World::GetOrCreateArchetype(const Signature& sig) 
{
    if (archetypeMap.find(sig) != archetypeMap.end()) {
        return archetypeMap[sig];
    }
    auto arch = std::make_shared<Archetype>(sig);
    archetypeMap[sig] = arch;
    return arch;
}

void ECS::World::MigrateEntity(EntityID id, EntityRecord& record, std::shared_ptr<Archetype> oldArch, std::shared_ptr<Archetype> newArch)
{
    size_t oldIdx = record.index;
    size_t newIdx = newArch->AddEntity(id);

    // 各コンポーネントの移動
    for (auto& pair : oldArch->columns) 
    {
        ComponentTypeID type = pair.first;

        // 新しいアーキタイプに含まれないコンポーネントは移動しない
        if (!std::binary_search(newArch->signature.begin(), newArch->signature.end(), type))
            continue;
            
        // 新しいアーキタイプに列(Array)がなければ作成
        if (newArch->columns.find(type) == newArch->columns.end()) {
            newArch->columns[type] = pair.second->CreateNew();
        }

        // データの移動
        pair.second->MoveData(oldIdx, newArch->columns[type].get(), newIdx);
    }

    // 古いアーキタイプから削除
    auto result = oldArch->RemoveEntity(oldIdx);

    // Swap削除で移動が発生した場合、そのエンティティのIndexを更新
    if (result.first) {
        entityIndex[result.second].index = oldIdx;
    }

    // 自身のレコード更新
    record.archetype = newArch;
    record.index = newIdx;
}
