#include "Engine_pch.h"
#include "SceneSerializer.h"
#include "CommandBuffer.h"
#include "GraphicsCore.h"
#include "UUID.h"
#include "Camera.h"
#include "CameraSystem.h"
#include "MeshRenderer.h"
#include "SpriteRenderer.h"
#include "ColliderSystem.h"
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <utility>

std::vector<SceneSerializer::ComponentSerialization>& SceneSerializer::GetComponents()
{
    // 登録されたコンポーネントの保存・復元処理を保持する
    static std::vector<ComponentSerialization> components;
    return components;
}

json SceneSerializer::Capture(ECS::World* world)
{
    json sceneJson;
    sceneJson["Entities"] = json::array();

    // メインカメラはエンティティIDではなくUUIDで保存する
    uint64_t mainCameraUuid = 0;
    const ECS::EntityID cameraId = CameraSystem::GetCamera();
    if (world->HasComponent<Camera>(cameraId) &&
        world->HasComponent<UUIDComponent>(cameraId))
    {
        mainCameraUuid = world->GetComponent<UUIDComponent>(cameraId).id;
    }
    sceneJson["MainCameraUUID"] = mainCameraUuid;

    // すべてのエンティティとコンポーネントをJsonに保存する
    for (ECS::EntityID id : world->GetEntityIds())
    {
        json entityJson;
        entityJson["EntityID"] = static_cast<uint32_t>(id);
        entityJson["Components"] = json::object();
        for (const auto& info : GetComponents())
        {
            info.serialize(world, id, entityJson);
        }
        sceneJson["Entities"].push_back(std::move(entityJson));
    }
    return sceneJson;
}

bool SceneSerializer::IsValid(const json& sceneJson)
{
    // エンティティの配列があるか確認する
    if (!sceneJson.is_object() ||
        !sceneJson.contains("Entities") ||
        !sceneJson["Entities"].is_array())
    {
        return false;
    }
    // メインカメラのUUIDがある場合は型を確認する
    if (sceneJson.contains("MainCameraUUID") &&
        !sceneJson["MainCameraUUID"].is_number_unsigned())
    {
        return false;
    }
    // すべてのエンティティにコンポーネントのオブジェクトがあるか確認する
    for (const auto& entityJson : sceneJson["Entities"])
    {
        if (!entityJson.is_object() ||
            !entityJson.contains("Components") ||
            !entityJson["Components"].is_object())
        {
            return false;
        }
    }
    return true;
}

bool SceneSerializer::Restore(ECS::World* world, const json& sceneJson,
    CommandBuffer* commandBuffer)
{
    // 復元先とJsonの構造を確認する
    if (!world || !IsValid(sceneJson))
    {
        return false;
    }

    // 保存されているメインカメラのUUIDを取得する
    const bool hasMainCameraUuid = sceneJson.contains("MainCameraUUID");
    const uint64_t mainCameraUuid = hasMainCameraUuid ?
        sceneJson["MainCameraUUID"].get<uint64_t>() : 0;

    // 描画で使用中のリソースを破棄する前にGPUを待つ
    auto& gfx = GraphicsCore::Get();
    gfx.FlushCommandQueue();
    CameraSystem::SetCamera(ECS::INVALID_ENTITY_ID); // 現在のメインカメラを解除する

    // 今あるシーンのコマンドとエンティティを削除する
    if (commandBuffer)
    {
        commandBuffer->Clear();
    }
    world->Clear();
    ColliderSystem::Instance().ClearContactHistory(); // 当たり判定の履歴もクリア
    if (commandBuffer)
    {
        // スクリプトの終了処理で積まれたコマンドも破棄する
        commandBuffer->Clear();
    }

    // 復元するリソースの転送コマンドを開始する
    auto ictx = gfx.GetCommandContext();
    ictx.BeginFrame(gfx.GetCurrentCommandAllocator());
    // エンティティを作成して保存されているコンポーネントを復元する
    for (const auto& entityJson : sceneJson["Entities"])
    {
        const ECS::EntityID newId = world->CreateEntity();
        for (const auto& info : GetComponents())
        {
            info.deserialize(world, newId, entityJson);
        }
    }
    // 転送コマンドを実行してGPUの処理が終わるまで待つ
    ictx.EndFrame();
    gfx.FlushCommandQueue();

    // 転送完了後に中間バッファを解放する
    world->ForEachComponent<MeshRenderer>(
        [](ECS::EntityID id, MeshRenderer& mesh) {
            if (mesh.model.asset)
            {
                mesh.model.asset->FreeUploadBuffers();
            }
        });
    world->ForEachComponent<SpriteRenderer>(
        [](ECS::EntityID id, SpriteRenderer& sprite) {
            if (sprite.texture.asset)
            {
                sprite.texture.asset->FreeUploadBuffer();
            }
        });

    ResolveLinks(world); // エンティティ間の参照を復元する

    ECS::EntityID restoredCameraId = ECS::INVALID_ENTITY_ID;
    if (hasMainCameraUuid)
    {
        // 保存されているUUIDに一致するカメラを選ぶ
        world->ForEachComponent<UUIDComponent>(
            [&](ECS::EntityID id, UUIDComponent& uuid) {
                if (mainCameraUuid != 0 && uuid.id == mainCameraUuid &&
                    world->HasComponent<Camera>(id))
                {
                    restoredCameraId = id;
                }
            });
    }
    else
    {
        // 旧形式ではID順で最初のカメラを使用する
        for (ECS::EntityID id : world->GetEntityIds())
        {
            if (world->HasComponent<Camera>(id))
            {
                restoredCameraId = id;
                break;
            }
        }
    }
    CameraSystem::SetCamera(restoredCameraId); // 復元したメインカメラを設定する
    return true;
}

bool SceneSerializer::Save(ECS::World* world, const std::string& filepath,
    std::string& error)
{
    std::filesystem::path temporaryPath;
    bool hasTemporaryFile = false;
    try
    {
        // 現在のシーンをJsonの文字列に変換する
        const auto targetPath = std::filesystem::u8path(filepath);
        const std::string contents = Capture(world).dump(4);
        // 保存先のフォルダがなければ作成する
        if (targetPath.has_parent_path())
        {
            std::filesystem::create_directories(targetPath.parent_path());
        }

        // 保存が完了するまで既存ファイルを置き換えない
        temporaryPath = targetPath;
        temporaryPath += L".tmp";
        if (std::filesystem::exists(temporaryPath))
        {
            throw std::runtime_error("A temporary save file already exists.");
        }
        // 一時ファイルを書き込み用に開く
        std::ofstream file(temporaryPath, std::ios::binary);
        hasTemporaryFile = file.is_open();
        if (!hasTemporaryFile)
        {
            throw std::runtime_error("Cannot create the scene file.");
        }
        file << contents; // シーンのJsonを書き込む
        file.close();
        // 書き込みに成功したら一時ファイルを保存先に置き換える
        if (!file || !MoveFileExW(temporaryPath.c_str(), targetPath.c_str(),
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        {
            throw std::runtime_error("Cannot finish saving the scene.");
        }

        hasTemporaryFile = false; // 移動済みの一時ファイルを削除対象から外す
        error.clear();
        return true;
    }
    catch (const std::exception& exception)
    {
        // 保存に失敗した場合は作成した一時ファイルを削除する
        if (hasTemporaryFile)
        {
            std::error_code cleanupError;
            std::filesystem::remove(temporaryPath, cleanupError);
        }
        error = std::string("Save failed: ") + exception.what(); // エラー登録
        return false;
    }
}

bool SceneSerializer::Load(ECS::World* world, const std::string& filepath,
    CommandBuffer* commandBuffer, std::string& error)
{
    try
    {
        // シーンファイルを読み込み用に開く
        std::ifstream file(std::filesystem::u8path(filepath), std::ios::binary);
        if (!file)
        {
            throw std::runtime_error("Cannot open the scene file.");
        }
        // ファイルからJsonを読み込んでシーンを復元する
        const json sceneJson = json::parse(file, nullptr, false);
        if (!Restore(world, sceneJson, commandBuffer))
        {
            throw std::runtime_error("Invalid scene data.");
        }

        error.clear();
        return true;
    }
    catch (const std::exception& exception)
    {
        error = std::string("Open failed: ") + exception.what(); // エラー登録
        return false;
    }
}

void SceneSerializer::ResolveLinks(ECS::World* world)
{
    // UUIDから現在のエンティティIDを引けるように対応表を作る
    std::unordered_map<uint64_t, ECS::EntityID> uuidToEntityMap;
    world->ForEachComponent<UUIDComponent>(
        [&](ECS::EntityID id, UUIDComponent& uuid) {
            uuidToEntityMap[uuid.id] = id;
        });

    // 対応表を使って各コンポーネントのエンティティ参照を復元する
    for (ECS::EntityID id : world->GetEntityIds())
    {
        for (const auto& info : GetComponents())
        {
            info.resolveLinks(world, id, uuidToEntityMap);
        }
    }
}
