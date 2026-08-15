#include "Archetype.h"

size_t ECS::Archetype::AddEntity(EntityID id)
{
    entities.push_back(id);
    return entities.size() - 1;
}

std::pair<bool, ECS::EntityID> ECS::Archetype::RemoveEntity(size_t index)
{
    size_t lastIndex = entities.size() - 1;
    EntityID movedEntity = entities[lastIndex];

    // 全カラムで削除実行
    for (auto& pair : columns) 
    {
        pair.second->RemoveSwap(index);
    }

    // IDリストの削除
    if (index != lastIndex) 
    {
        entities[index] = entities[lastIndex];
        entities.pop_back();
        return { true, movedEntity }; // 末尾が移動してきた
    }
    else 
    {
        entities.pop_back();
        return { false, 0 }; // 末尾が消えただけ
    }
}
