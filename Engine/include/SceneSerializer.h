#pragma once

#include "Archive.h"
#include "world.h"
#include <algorithm>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
#include <utility>

class CommandBuffer;

// エディタとゲーム実行時で共有するシーンの保存・復元処理
class SceneSerializer
{
public:
    template<typename T>
    static void RegisterComponent(const std::string& name)
    {
        ComponentSerialization info;
        info.name = name;
        info.componentTypeId = ECS::ComponentRegistry::GetID<T>();
        // JSONへ保存
        info.serialize = [name](ECS::World* world, ECS::EntityID id, json& outJson) {
            if (world->HasComponent<T>(id)) {
                auto& comp = world->GetComponent<T>(id);
                JsonWriteArchive archive;
                comp.Reflect(archive);
                outJson["Components"][name] = archive.j;
            }
            };

        // JSONから復元
        info.deserialize = [name](ECS::World* world, ECS::EntityID id, const json& inJson) {
            if (inJson.contains("Components") && inJson["Components"].contains(name)) {
                T comp{};
                if (world->HasComponent<T>(id)) {
                    comp = world->GetComponent<T>(id);
                }
                JsonReadArchive archive(inJson["Components"][name]);
                comp.Reflect(archive);
                world->AddComponent<T>(id, comp);
            }
            };

        // リンクの解決
        info.resolveLinks = [name](ECS::World* world, ECS::EntityID id, const std::unordered_map<uint64_t, ECS::EntityID>& uuidMap) {
            if (world->HasComponent<T>(id)) {
                auto& comp = world->GetComponent<T>(id);
                // リンク解決専用アーカイバ
                ResolveArchive archive(uuidMap);
                comp.Reflect(archive);
            }
            };

        // シーン初期化時に再登録されても重複させない
        auto& components = GetComponents();
        const auto entry = std::find_if(components.begin(), components.end(),
            [&](const ComponentSerialization& registered) {
                return registered.componentTypeId == info.componentTypeId ||
                    registered.name == info.name;
            });
        if (entry != components.end())
        {
            *entry = std::move(info);
        }
        else
        {
            components.push_back(std::move(info));
        }
    }

    static json Capture(ECS::World* world);                 // シーンをJsonにする
    static bool IsValid(const json& sceneJson);
    static bool Restore(ECS::World* world, const json& sceneJson,CommandBuffer* commandBuffer); // Jsonからシーン作る
    static bool Save(ECS::World* world, const std::string& filepath,std::string& error);        // Jsonファイルにセーブする
    static bool Load(ECS::World* world, const std::string& filepath,CommandBuffer* commandBuffer, std::string& error);  // Jsonファイルから復元する

    // エンティティ削除後などにUUIDから参照を解決し直す
    static void ResolveLinks(ECS::World* world);

private:
    struct ComponentSerialization
    {
        std::string name;
        ECS::ComponentTypeID componentTypeId = 0;
        std::function<void(ECS::World*, ECS::EntityID, json&)> serialize;
        std::function<void(ECS::World*, ECS::EntityID, const json&)> deserialize;
        std::function<void(ECS::World*, ECS::EntityID,
            const std::unordered_map<uint64_t, ECS::EntityID>&)> resolveLinks;
    };

    static std::vector<ComponentSerialization>& GetComponents();
};