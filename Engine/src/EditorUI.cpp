#include "Engine_pch.h"

#include "EditorUI.h"
#include "GraphicsCore.h"
#include <stdexcept>
#include "Name.h" 

#include <fstream>
#include <iomanip> // 綺麗に改行してJSONを保存するため
#include "UUID.h"
#include "MeshRenderer.h"
#include "SpriteRenderer.h"


void EditorUI::Initialize(HWND hwnd) {
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

            // Save ボタン
            if (ImGui::MenuItem("Save Scene")) {
                // プロジェクトフォルダのルートに scene.json として保存
                SaveScene(world, "Assets\\scene\\scene.json");
            }

            if (ImGui::MenuItem("Load Scene")) {
                nextScene = "Assets\\scene\\scene.json";
            }

            ImGui::EndMenu();
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

        // ！！超重要！！
        // 前のフレームのGPU描画が完全に終わるのを待機する
        GraphicsCore::Get().FlushCommandQueue();

        // 実際にシーンを破棄して読み込む
        LoadScene(world, nextScene);

        // 予約フラグをクリア
        nextScene.clear();
    }
}

void EditorUI::SaveScene(ECS::World* world, const std::string& filepath)
{
    json sceneJson;
    sceneJson["Entities"] = json::array(); // エンティティの配列を作成

    // NameComponent を持つ全エンティティを保存対象としてループ
    world->ForEachComponent<Name>([&](ECS::EntityID id, Name& nameComp) {

        json entityJson;
        entityJson["EntityID"] = static_cast<uint32_t>(id);

        // 登録されている全コンポーネントの serialize を順番に呼ぶ
        for (auto& info : componentRegistry) {
            info.serialize(world, id, entityJson);
        }

        // 配列に追加
        sceneJson["Entities"].push_back(entityJson);
        });

    // ファイルに書き出し (dump(4) でインデントを4マスにして見やすくする)
    std::ofstream file(filepath);
    if (file.is_open()) {
        file << sceneJson.dump(4);
        file.close();
    }
}

void EditorUI::LoadScene(ECS::World* world, const std::string& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open()) {
        // ファイルが無ければ何もしない
        return;
    }

    json sceneJson;
    file >> sceneJson;
    file.close();

    if (!sceneJson.contains("Entities")) return;

    // 現在のシーンにあるオブジェクトをすべて削除する（クリーンアップ）
    std::vector<ECS::EntityID> entitiesToDelete;

    // NameComponentを持っている全エンティティをリストアップ
    world->ForEachComponent<Name>([&](ECS::EntityID id, Name& nameComp) {
        entitiesToDelete.push_back(id);
        });

    // リストアップしたエンティティを削除
    for (auto id : entitiesToDelete) {
        world->DeleteEntity(id);
    }

    // 選択状態をリセットして安全を確保
    selectedEntityID = ECS::INVALID_ENTITY_ID;

        auto& gfx = GraphicsCore::Get();
    auto ictx = gfx.GetCommandContext();
    ictx.BeginFrame(gfx.GetCurrentCommandAllocator());

    // JSONからエンティティを復元する
    for (const auto& entityJson : sceneJson["Entities"]) {
        ECS::EntityID newId = world->CreateEntity();
        for (auto& info : componentRegistry) {
            info.deserialize(world, newId, entityJson);
        }
    }

    ictx.EndFrame();
    gfx.FlushCommandQueue();

    // アップロードバッファの解放
    world->ForEachComponent<MeshRenderer>([](ECS::EntityID id, MeshRenderer& mr) {
        if (mr.model.asset) mr.model.asset->FreeUploadBuffers();
    });
    world->ForEachComponent<SpriteRenderer>([](ECS::EntityID id, SpriteRenderer& sr) {
        if (sr.texture.asset) sr.texture.asset->FreeUploadBuffer();
    });

    // UUID から 実際の新しい EntityID を見つけるための辞書
    std::unordered_map<uint64_t, ECS::EntityID> uuidToEntityMap;

    // 現在シーンにいる全エンティティを走査して辞書に登録する
    world->ForEachComponent<UUIDComponent>([&](ECS::EntityID id, UUIDComponent& uuidComp) {
        uuidToEntityMap[uuidComp.id] = id;
        });

    // すべてのコンポーネントの参照を解決する
    world->ForEachComponent<UUIDComponent>([&](ECS::EntityID id, UUIDComponent& uuidComp) {
        for (auto& info : componentRegistry) {
            // 各コンポーネント内のEntityRefを更新
            info.resolveLinks(world, id, uuidToEntityMap);
        }
        });
}

void EditorUI::DrawHierarchyWindow(ECS::World* world) {
    ImGui::Begin("Hierarchy");

    if (ImGui::Button("Create Empty Object")) {
        auto newEntity = world->CreateEntity();
        world->AddComponent(newEntity, UUIDComponent{});
        world->AddComponent(newEntity, Name{ "New Object" });
    }

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