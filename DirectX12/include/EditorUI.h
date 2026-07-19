#pragma once
//#include <imgui/imgui.h>
//#include <imgui/imgui_impl_win32.h>
//#include <imgui/imgui_impl_dx12.h>
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx12.h>
#include <string>
#include <vector>
#include <DirectXMath.h>

// シーンに配置するオブジェクトの簡易表現
struct SceneObject {
    std::string Name;
    DirectX::XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 Rotation = { 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 Scale = { 1.0f, 1.0f, 1.0f };
    bool IsVisible = true;
};

class EditorUI {
public:
    void Initialize(HWND hwnd);
    void Shutdown();

    void BeginUI();
    void RenderUI();
    void DrawImGui();

private:
    void DrawHierarchyWindow();
    void DrawInspectorWindow();

    std::vector<SceneObject> sceneObjects;
    int selectedObjectIndex = -1;
};