// ComponentMetaData.h
#pragma once
#include "world.h" 
#include "json.hpp"
#include <functional>
#include <string>

using json = nlohmann::json;

struct ComponentMetaData {
    std::string name;

    // UI関連
    std::function<void(ECS::World*, ECS::EntityID)> drawInspector;
    std::function<void(ECS::World*, ECS::EntityID)> addComponent;

    // シリアライズ関連
    std::function<void(ECS::World*, ECS::EntityID, json&)> serialize;
    std::function<void(ECS::World*, ECS::EntityID, const json&)> deserialize;

    std::function<void(ECS::World*, ECS::EntityID, const std::unordered_map<uint64_t, ECS::EntityID>&)> resolveLinks;
};