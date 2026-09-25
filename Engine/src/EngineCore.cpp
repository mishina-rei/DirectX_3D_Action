#include "Engine_pch.h"
#include "EngineCore.h"
#include <crtdbg.h>
#pragma comment(lib, "winmm.lib")

#include "Sprite.h"
#include "Input.h"
#include "Audio.h"
#include "EffekseerManager.h"
#include "RenderSystem.h"

#include "GraphicsCore.h" 
#include "EditorUI.h"

// ImGuiのWin32メッセージハンドラを宣言
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


bool EngineCore::Initialize(const std::wstring& title, int width, int height)
{
    
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    m_width = width;
    m_height = height;

    // ウィンドウサイズ固定する
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WindowProc;
    wcex.hInstance = GetModuleHandle(nullptr);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.lpszClassName = L"EngineWindowClass";
    RegisterClassExW(&wcex);

    RECT rect = { 0, 0, width, height };
    DWORD style = WS_OVERLAPPEDWINDOW;
    AdjustWindowRectEx(&rect, style, false, WS_EX_OVERLAPPEDWINDOW);

    // CreateWindowExA (マルチバイト版) を明示的に呼ぶ
    m_hwnd = CreateWindowExW(
        WS_EX_OVERLAPPEDWINDOW, wcex.lpszClassName, title.c_str(), style,
        CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top,
        nullptr, nullptr, wcex.hInstance, nullptr
    );

    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);

    // エンジン各種システムの初期化
    GraphicsCore::Get().Initialize(m_hwnd, width, height);

	// シェーダーのロード
    ShaderManager::Get().LoadShader("VS_Sprite", L"C:\\Develop\\3D_Action\\DirectX_3D_Action\\3D_Action\\src\\Shader\\SpriteVS.hlsl", L"main", L"vs_6_0");
    ShaderManager::Get().LoadShader("PS_Sprite", L"C:\\Develop\\3D_Action\\DirectX_3D_Action\\3D_Action\\src\\Shader\\SpritePS.hlsl", L"main", L"ps_6_0");
    Sprite::Init();
    InitInput(m_hwnd);
    InitAudio();
    EffekseerManager::Init();
    RenderSystem::Init(); // シャドウマップの初期化等

    // エディタUIの初期化
    EditorUI::Get().Initialize(m_hwnd);

    return true;
}

void EngineCore::MainLoop()
{
    MSG message = {};
    timeBeginPeriod(1);
    DWORD preExecTime = timeGetTime();

    while (true)
    {
        if (PeekMessage(&message, NULL, 0, 0, PM_NOREMOVE))
        {
            if (!GetMessage(&message, NULL, 0, 0)) break;
            TranslateMessage(&message);
            DispatchMessage(&message);
        }
        else
        {
            DWORD nowTime = timeGetTime();
            if (static_cast<float>(nowTime - preExecTime) >= 1000.0f / 60.0f)
            {
                UpdateInput();

                EditorUI::Get().Update(SceneManager::GetInstance().GetCurrentScene()->GetWorld());

                SceneManager::GetInstance().Update();
                EffekseerManager::Update();

                // DX12のフレーム制御を組み込む
                GraphicsCore::Get().BeginFrame();

                // シーン内のすべての描画コマンドを積む
                SceneManager::GetInstance().Draw();

				EditorUI::Get().BeginUI();
				EditorUI::Get().RenderUI(SceneManager::GetInstance().GetCurrentScene()->GetWorld());

                GraphicsCore::Get().EndFrame();

                preExecTime = nowTime;
            }
        }
    }
    timeEndPeriod(1);
}

void EngineCore::Terminate()
{
    // GPUの描画処理が完全に終わるのを待つ
    GraphicsCore::Get().FlushCommandQueue();
    
    // エディタUIの終了処理
    EditorUI::Get().Shutdown();

    SceneManager::GetInstance().Uninit();
    EffekseerManager::Uninit();
    UninitAudio();
    UninitInput();
    Sprite::Uninit();

    UnregisterClassA("EngineWindowClass", GetModuleHandle(nullptr));
}

LRESULT CALLBACK EngineCore::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // ImGuiにメッセージを投げる（UIがクリックなどを処理するため）
    if (ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().BackendPlatformUserData != nullptr) {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam)) {
            return true;
        }

        // ImGuiが「キーボードやマウスを使っている」場合は、ゲーム側の入力処理を弾く
        ImGuiIO& io = ImGui::GetIO();

        // UI操作中なら、マウス入力関連のWindowsメッセージをゲーム側に送らない
        if (io.WantCaptureMouse && (message >= WM_MOUSEFIRST && message <= WM_MOUSELAST)) {
            return 1;
        }
        // UI操作中なら、キーボード入力関連のメッセージをゲーム側に送らない
        if (io.WantCaptureKeyboard && (message >= WM_KEYFIRST && message <= WM_KEYLAST)) {
            return 1;
        }
    }


    // アプリケーション独自のメッセージ処理
    switch (message) {
    case WM_DESTROY:
        PostQuitMessage(0); // ウィンドウの「×」ボタンが押されたら終了メッセージを投げる
        return 0;

    }

    // 上記以外のメッセージはOSのデフォルト処理に任せる
    return DefWindowProc(hWnd, message, wParam, lParam);
}