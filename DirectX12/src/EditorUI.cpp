#include "DirectX12_pch.h"

#include "EditorUI.h"
#include "GraphicsCore.h"
#include <stdexcept>


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

    // テスト用の初期オブジェクト
    sceneObjects.push_back({ "Player", {0, 0, 0} });
    sceneObjects.push_back({ "Enemy", {5, 0, 0} });
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

void EditorUI::RenderUI()
{

    DrawHierarchyWindow();
    DrawInspectorWindow();

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

void EditorUI::DrawHierarchyWindow() {
    ImGui::Begin("Hierarchy");

    if (ImGui::Button("Create Empty Object")) {
        sceneObjects.push_back({ "New Object " + std::to_string(sceneObjects.size()) });
    }

    ImGui::Separator();

    for (int i = 0; i < sceneObjects.size(); i++) {
        bool isSelected = (selectedObjectIndex == i);
        if (ImGui::Selectable(sceneObjects[i].Name.c_str(), isSelected)) {
            selectedObjectIndex = i;
        }
    }
    ImGui::End();
}

void EditorUI::DrawInspectorWindow() {
    ImGui::Begin("Inspector");

    if (selectedObjectIndex >= 0 && selectedObjectIndex < sceneObjects.size()) {
        auto& obj = sceneObjects[selectedObjectIndex];

        // 名前
        char nameBuf[128];
        strcpy_s(nameBuf, obj.Name.c_str());
        if (ImGui::InputText("Name", nameBuf, IM_ARRAYSIZE(nameBuf))) {
            obj.Name = nameBuf;
        }

        ImGui::Separator();

        // トランスフォーム        
        ImGui::DragFloat3("Position", &obj.Position.x, 0.1f);
        ImGui::DragFloat3("Rotation", &obj.Rotation.x, 1.0f);
        ImGui::DragFloat3("Scale", &obj.Scale.x, 0.1f);

        ImGui::Checkbox("Visible", &obj.IsVisible);
    }
    else {
        ImGui::Text("No object selected.");
    }

    ImGui::End();
}