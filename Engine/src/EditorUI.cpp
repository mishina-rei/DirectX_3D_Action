#include "Engine_pch.h"

#include "EditorUI.h"
#include "GraphicsCore.h"
#include <stdexcept>
#include <algorithm>
#include "Name.h" 

#include <fstream>
#include <utility>
#include <iomanip> // 綺麗に改行してJSONを保存するため
#include "UUID.h"
#include "MeshRenderer.h"
#include "SpriteRenderer.h"
#include "Camera.h"
#include "CameraSystem.h"
#include "SceneManager.h"
#include "ColliderSystem.h"
#include "Script.h"
#include "EngineTime.h"
#include "Input.h"
#include "EffekseerManager.h"


void EditorUI::Initialize(HWND hwnd) {
    editorState = EditorState::Edit;
    pendingDeleteEntity = ECS::INVALID_ENTITY_ID;
    pendingAction = EditorAction::None;
    editSceneSnapshot = json();
    playbackError.clear();
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;       // ドッキング機能を有効化
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;   // キーボード有効化

    ImGui::StyleColorsDark();   // 色

    auto& gfx = GraphicsCore::Get();

    bool is = false;
    bool is32 = false;

    // ImGui用のSRVディスクリプタヒープを渡す
    handle = gfx.GetSrvHeapManager().Allocate();

    ImGui_ImplDX12_InitInfo init_info = {};
    init_info.Device = gfx.GetDevice();
    init_info.NumFramesInFlight = 2; // GraphicsCore::frameCount と一致させる
    init_info.RTVFormat = gfx.GetBackBufferFormat();
    init_info.CommandQueue = gfx.GetCommandQueue(); // グラフィックスコアのコマンドキューを共有
    init_info.SrvDescriptorHeap = gfx.GetSrvHeapManager().GetHeap();
    init_info.LegacySingleSrvCpuDescriptor = handle.CPUHandle; // 古いAPIとの互換性のためにこれらを使用
    init_info.LegacySingleSrvGpuDescriptor = handle.GPUHandle;

    is32 = ImGui_ImplWin32_Init(hwnd);
    is = ImGui_ImplDX12_Init(&init_info);

    ImGui::GetIO().Fonts->Build();
    if (!ImGui_ImplDX12_CreateDeviceObjects()) {
        throw std::runtime_error("ImGui");
    }
}

void EditorUI::Shutdown()
{
    // 終了処理
    if (handle.IsValid())
        GraphicsCore::Get().GetSrvHeapManager().Free(handle);
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void EditorUI::BeginUI()
{
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    // 全体をドッキングスペースにする
    //ImGui::DockSpaceOverViewport();
    ImGui::DockSpaceOverViewport(0,nullptr, ImGuiDockNodeFlags_PassthruCentralNode);
}

void EditorUI::RenderUI(ECS::World* world)
{
    // エディタのメインメニューバーを描画
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            const bool canEditScene = editorState == EditorState::Edit;

            // Save ボタン
            if (ImGui::MenuItem("Save Scene", nullptr, false, canEditScene)) {
                // プロジェクトフォルダのルートに scene.json として保存
                SaveScene(world, "Assets\\scene\\scene.json");
            }

            if (ImGui::MenuItem("Load Scene", nullptr, false, canEditScene)) {
                nextScene = "Assets\\scene\\scene.json";
            }

            ImGui::EndMenu();
        }
        ImGui::Separator();

		switch (editorState)    // エディタの状態に応じてボタンを切り替える
        {
        case EditorState::Edit:
            if (ImGui::Button("Play"))
            {
                pendingAction = EditorAction::Play;
            }
            break;

        case EditorState::Playing:
            if (ImGui::Button("Pause"))
            {
                editorState = EditorState::Paused;
            }
            break;

        case EditorState::Paused:
            if (ImGui::Button("Resume"))
            {
                editorState = EditorState::Playing;
            }
            break;
        }

        if (editorState != EditorState::Edit)
        {
            ImGui::SameLine();

            if (ImGui::Button("Stop"))
            {
                pendingAction = EditorAction::Stop;
            }
        }

        ImGui::SameLine();

		// エディタの状態を表示
        const char* stateName = "Edit";

        if (editorState == EditorState::Playing)
        {
            stateName = "Playing";
        }
        else if (editorState == EditorState::Paused)
        {
            stateName = "Paused";
        }

        ImGui::TextUnformatted(stateName);

		// エラーがあれば表示する
        if (!playbackError.empty())
        {
            ImGui::SameLine();
            ImGui::TextUnformatted(playbackError.c_str());
        }
        ImGui::EndMainMenuBar();
    }

    ImGui::Text("DEBUG: Current Selected ID = %lld", (long long)selectedEntityID);
    DrawHierarchyWindow(world);
    DrawInspectorWindow(world);

    // 描画データ生成
    ImGui::Render();

    auto* commandList = GraphicsCore::Get().GetCommandList();
    //auto handle = GraphicsCore::Get().GetRtvHandle();
    //commandList->OMSetRenderTargets(1, &handle, FALSE, nullptr);

    // ImGuiの描画コマンドを積む前に、専用のディスクリプタヒープをセットする
    ID3D12DescriptorHeap* heaps[] = { GraphicsCore::Get().GetSrvHeapManager().GetHeap()};
    commandList->SetDescriptorHeaps(1, heaps);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
}

void EditorUI::Update(ECS::World* world)
{
    // ロード予約が入っていたら実行する
    if (!nextScene.empty()) {
        pendingDeleteEntity = ECS::INVALID_ENTITY_ID;

        // 実際にシーンを破棄して読み込む
        LoadScene(world, nextScene);

        // 予約フラグをクリア
        nextScene.clear();
    }
    else if (pendingDeleteEntity != ECS::INVALID_ENTITY_ID)
    {
        const ECS::EntityID id = pendingDeleteEntity;
        pendingDeleteEntity = ECS::INVALID_ENTITY_ID;

        if (editorState == EditorState::Edit)
        {
			DeleteEditorEntity(world, id);  // エディタ上でエンティティを削除する
        }
    }

    const EditorAction action = pendingAction;
    pendingAction = EditorAction::None;

	// 再生・停止の予約アクションがあれば実行する
    switch (action)
    {
    case EditorAction::Play:
        StartPlay(world);
        break;

    case EditorAction::Stop:
        StopPlay(world);
        break;

    case EditorAction::None:
        break;
    }
}

void EditorUI::StartPlay(ECS::World* world)
{
    if (editorState != EditorState::Edit)
    {
        return;
    }

	playbackError.clear();  // エラーをクリア

    bool canStartPlay = true;

    world->ForEachComponent<Script>(
        [&](ECS::EntityID id, Script& script)
        {
			if (script.HasUnavailableType())    // 型名が存在しない場合は再生できない
            {
                canStartPlay = false;
            }
        });

    if (!canStartPlay)
    {
        playbackError = "Cannot play: a script is unavailable.";    // エラー登録
        return;
    }

	editSceneSnapshot = CaptureScene(world);    // 現在のシーンをメモリに保存
    editTimeScale = Time::GetTimeScale();

    editorState = EditorState::Playing;
}

void EditorUI::StopPlay(ECS::World* world)
{
    if (editorState == EditorState::Edit)
    {
        return;
    }

    editorState = EditorState::Paused;
    SetMouseLock(false);

    if (!RestoreScene(world, editSceneSnapshot))
    {
		playbackError = "Failed to restore the edit scene.";    // エラー登録
        return;
    }

	EffekseerManager::StopAll();        // 再生中のエフェクトをすべて停止
	Time::SetTimeScale(editTimeScale);  // 時間のスケールを元に戻す

	editSceneSnapshot = json(); // メモリに保存していたシーンデータをクリア
    playbackError.clear();
    editorState = EditorState::Edit;
}

json EditorUI::CaptureScene(ECS::World* world) const
{
    json sceneJson;
    sceneJson["Entities"] = json::array();

	// メインカメラのUUIDを保存する
    uint64_t mainCameraUuid = 0;
    const ECS::EntityID cameraId = CameraSystem::GetCamera();

    if (world->HasComponent<Camera>(cameraId) &&
        world->HasComponent<UUIDComponent>(cameraId))
    {
        mainCameraUuid = world->GetComponent<UUIDComponent>(cameraId).id;
    }

    sceneJson["MainCameraUUID"] = mainCameraUuid;

	// すべてのエンティティを走査してJSONに保存する
    for (ECS::EntityID id : world->GetEntityIds())
    {
        json entityJson;
        entityJson["EntityID"] = static_cast<uint32_t>(id);
        entityJson["Components"] = json::object();

        for (const auto& info : componentRegistry)
        {
            info.serialize(world, id, entityJson);
        }

        sceneJson["Entities"].push_back(std::move(entityJson));
    }

    return sceneJson;
}

void EditorUI::SaveScene(ECS::World* world, const std::string& filepath)
{
    const json sceneJson = CaptureScene(world);

    std::ofstream file(filepath);
    if (file.is_open())
    {
        file << sceneJson.dump(4);
    }
}

void EditorUI::LoadScene(ECS::World* world, const std::string& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        OutputDebugStringA("Failed to open scene file.\n");
        return;
    }

    const json sceneJson = json::parse(file, nullptr, false);

    if (!RestoreScene(world, sceneJson))
    {
        OutputDebugStringA("Invalid scene data.\n");
    }
}

bool EditorUI::RestoreScene(ECS::World* world, const json& sceneJson)
{
    // Jsonの構造を確認する
    if (!sceneJson.is_object() ||
        !sceneJson.contains("Entities") ||
        !sceneJson["Entities"].is_array())
    {
		return false;
    }

    const bool hasMainCameraUuid = sceneJson.contains("MainCameraUUID");
    uint64_t mainCameraUuid = 0;

    if (hasMainCameraUuid)
    {
        if (!sceneJson["MainCameraUUID"].is_number_unsigned())
        {
            return false;
        }

        mainCameraUuid = sceneJson["MainCameraUUID"].get<uint64_t>();
    }

    const auto& entities = sceneJson["Entities"];

    for (const auto& entityJson : entities)
    {
        if (!entityJson.is_object() ||
            !entityJson.contains("Components") ||
            !entityJson["Components"].is_object())
        {
            return false;
        }
    }

    auto& gfx = GraphicsCore::Get();
    gfx.FlushCommandQueue();

    // 構造が確認出来たら今あるシーンを消す
    selectedEntityID = ECS::INVALID_ENTITY_ID;
    pendingDeleteEntity = ECS::INVALID_ENTITY_ID;
    CameraSystem::SetCamera(ECS::INVALID_ENTITY_ID);

    auto* scene = SceneManager::GetInstance().GetCurrentScene();

    CommandBuffer* sceneCommands = nullptr;

    if (scene && scene->GetWorld() == world)
    {
        sceneCommands = scene->GetCommandBuffer();
		sceneCommands->Clear(); // 既存のコマンドをクリア
    }

	world->Clear();             // すべてのエンティティを削除
	ColliderSystem::Instance().ClearContactHistory();   // 当たり判定の履歴もクリア

    if (sceneCommands)
    {
		sceneCommands->Clear(); // シーンを破棄した時にためられたコマンドもクリア
    }

    auto ictx = gfx.GetCommandContext();
    ictx.BeginFrame(gfx.GetCurrentCommandAllocator());

    // エンティティを復元する
    for (const auto& entityJson : entities)
    {
        const ECS::EntityID newId = world->CreateEntity();

        for (const auto& info : componentRegistry)
        {
            info.deserialize(world, newId, entityJson);
        }
    }

    ictx.EndFrame();
    gfx.FlushCommandQueue();

    // モデルとテクスチャの中間バッファを解放
    world->ForEachComponent<MeshRenderer>(
        [](ECS::EntityID id, MeshRenderer& mesh)
        {
            if (mesh.model.asset)
            {
                mesh.model.asset->FreeUploadBuffers();
            }
        });
    world->ForEachComponent<SpriteRenderer>(
        [](ECS::EntityID id, SpriteRenderer& sprite)
        {
            if (sprite.texture.asset)
            {
                sprite.texture.asset->FreeUploadBuffer();
            }
        });

    // UUIDの対応表を復元する
    std::unordered_map<uint64_t, ECS::EntityID> uuidToEntityMap;

    world->ForEachComponent<UUIDComponent>(
        [&](ECS::EntityID id, UUIDComponent& uuid)
        {
            uuidToEntityMap[uuid.id] = id;
        });

    for (ECS::EntityID id : world->GetEntityIds())
    {
        for (const auto& info : componentRegistry)
        {
            info.resolveLinks(world, id, uuidToEntityMap);
        }
    }

    ECS::EntityID restoredCameraId = ECS::INVALID_ENTITY_ID;

    if (hasMainCameraUuid)
    {
        // UUIDからカメラを選ぶ
        const auto cameraEntry = uuidToEntityMap.find(mainCameraUuid);

        if (mainCameraUuid != 0 &&
            cameraEntry != uuidToEntityMap.end() &&
            world->HasComponent<Camera>(cameraEntry->second))
        {
            restoredCameraId = cameraEntry->second;
        }
    }
    else
    {
        // ID順で最初のカメラを選択する
        for (ECS::EntityID id : world->GetEntityIds())
        {
            if (world->HasComponent<Camera>(id))
            {
                restoredCameraId = id;
                break;
            }
        }
    }

    CameraSystem::SetCamera(restoredCameraId);

    return true;
}

void EditorUI::DeleteEditorEntity(ECS::World* world, ECS::EntityID id)
{
    const auto entityIds = world->GetEntityIds();

    if (std::find(entityIds.begin(), entityIds.end(), id) == entityIds.end())
    {
		return; // 既に削除されている場合は何もしない
    }

    GraphicsCore::Get().FlushCommandQueue();

    selectedEntityID = ECS::INVALID_ENTITY_ID;

    if (CameraSystem::GetCamera() == id)
    {
		CameraSystem::SetCamera(ECS::INVALID_ENTITY_ID);    // 削除するエンティティがカメラだった場合はカメラを解除する
    }

	world->DeleteEntity(id);    // エンティティを削除する
    ColliderSystem::Instance().ClearContactHistory();

    std::unordered_map<uint64_t, ECS::EntityID> uuidToEntityMap;

    world->ForEachComponent<UUIDComponent>(
        [&](ECS::EntityID entityId, UUIDComponent& uuid)
        {
            uuidToEntityMap[uuid.id] = entityId;
        });

    for (ECS::EntityID entityId : world->GetEntityIds())
    {
        for (const auto& info : componentRegistry)
        {
			info.resolveLinks(world, entityId, uuidToEntityMap);    // リンクの解決
        }
    }
}

void EditorUI::DrawHierarchyWindow(ECS::World* world) {
    ImGui::Begin("Hierarchy");

	// "Create Empty Object" ボタンを押すと新しいエンティティを作成する
    if (ImGui::Button("Create Empty Object")) {
        auto newEntity = world->CreateEntity();
        world->AddComponent(newEntity, UUIDComponent{});
        world->AddComponent(newEntity, Name{ "New Object" });
    }

    ImGui::SameLine();

	// "Delete" ボタンは、エディタが編集モードであり、かつエンティティが選択されている場合のみ有効にする
    const bool canDeleteEntity = editorState == EditorState::Edit && selectedEntityID != ECS::INVALID_ENTITY_ID;

    ImGui::BeginDisabled(!canDeleteEntity);

    if (ImGui::Button("Delete"))
    {
		pendingDeleteEntity = selectedEntityID; // 削除予約フラグを立てる
    }

    ImGui::EndDisabled();

    ImGui::Separator();

    // NameComponentを持っている全エンティティをリスト表示
    world->ForEachComponent<Name>([&](ECS::EntityID id, Name& nameComp) {

        // エンティティIDをImGuiの内部IDとして登録する
        ImGui::PushID(static_cast<int>(id));

        bool isSelected = (selectedEntityID == id);
        if (ImGui::Selectable(nameComp.name.c_str(), isSelected)) {
            selectedEntityID = id;
        }

        // 解除
        ImGui::PopID();

        });
    ImGui::End();
}

void EditorUI::DrawInspectorWindow(ECS::World* world) {
    ImGui::Begin("Inspector");

    if (selectedEntityID != ECS::INVALID_ENTITY_ID) {

        // 登録された全コンポーネントのUIを自動生成
        for (auto& info : componentRegistry) {
            info.drawInspector(world, selectedEntityID);
        }

        ImGui::Separator();

        // コンポーネント追加メニューも自動生成
        if (ImGui::Button("Add Component")) {
            ImGui::OpenPopup("AddComponentPopup");
        }
        if (ImGui::BeginPopup("AddComponentPopup")) {
            for (auto& info : componentRegistry) {
                info.addComponent(world, selectedEntityID);
            }
            ImGui::EndPopup();
        }
    }
    else {
        ImGui::Text("No object selected.");
    }

    ImGui::End();
}
