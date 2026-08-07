// Archetype.h

#pragma once
#include <unordered_map>
#include "Signature.h"
#include "ComponentTypeID.h"
#include "EntityID.h"
#include "ComponentArray.h"

namespace ECS {

    class Archetype 
    {
    public:
        Signature signature;
        // 型ID -> 配列 のマップ
        std::unordered_map<ComponentTypeID, std::shared_ptr<IComponentArray>> columns;
        std::vector<EntityID> entities;

        Archetype(Signature sig) : signature(sig) {}

        // エンティティIDをリストに追加
        size_t AddEntity(EntityID id);

        // エンティティIDを削除 (Swap & Pop)
        // 戻り値: {移動が発生したか, 移動してきたEntityID}
        std::pair<bool, EntityID> RemoveEntity(size_t index);
    };
}
