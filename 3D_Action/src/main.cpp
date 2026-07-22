#include <windows.h>
#include "GraphicsCore.h"
#include "EditorUI.h"
#include "ShaderManager.h"

// ImGuiのWin32メッセージハンドラを宣言
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ウィンドウプロシージャ（OSからのメッセージを受け取る関数）
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // ImGuiにメッセージを投げる（UIがクリックなどを処理するため）
    if (ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().BackendPlatformUserData != nullptr) {
        if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam)) {
            return true;
        }
    }

    // アプリケーション独自のメッセージ処理
    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0); // ウィンドウの「×」ボタンが押されたら終了メッセージを投げる
        return 0;

    }

    // 上記以外のメッセージはOSのデフォルト処理に任せる
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    // ウィンドウサイズ固定する
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    // ウィンドウ作成
    const wchar_t* className = L"DX12GameEngineClass";
    const wchar_t* windowName = L"My Custom DX12 Engine";
    const uint32_t width = 1920;
    const uint32_t height = 1080;

    // ウィンドウクラスの設定
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;       // ウィンドウプロシージャを登録
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = className;

    RegisterClassExW(&wc);

    // ウィンドウサイズの補正（タイトルバーや枠の分を計算して、描画領域をぴったり1280x720にする）
    RECT rc = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    // ウィンドウの生成
    HWND hwnd = CreateWindowExW(
        0,
        className,
        windowName,
        WS_OVERLAPPEDWINDOW, // 通常のウィンドウ（リサイズ可能）
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left,  // 補正後の幅
        rc.bottom - rc.top,  // 補正後の高さ
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (!hwnd) {
        return -1;
    }

    // ウィンドウを表示
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // エンジンの初期化
    GraphicsCore::Get().Initialize(hwnd, 1920, 1080);

    // エディタ初期化
    EditorUI editor;
    editor.Initialize(hwnd);

    ShaderManager::Get().Initialize(GraphicsCore::Get().GetDevice());
    
    ShaderManager::Get().LoadShader("Standard", L"C:\\Develop\\3D_Action\\DirectX_3D_Action\\3D_Action\\src\\Shader\\StandardVS.hlsl", L"C:\\Develop\\3D_Action\\DirectX_3D_Action\\3D_Action\\src\\Shader\\StandardPS.hlsl");
    //ShaderManager::Get().LoadShader("Standard", L"Shader/StandardVS.hlsl", L"Shader/StandardPS.hlsl");
    ShaderManager::Get().CreateStandardPSO("Standard", GraphicsCore::Get().GetBackBufferFormat(), GraphicsCore::Get().GetDepthBufferFormat());

    // メインループ
    bool isRunning = true;
    while (isRunning) {
        MSG msg = {};
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            // 「×」ボタン等で終了メッセージ(WM_QUIT)を受け取ったらループを抜ける
            if (msg.message == WM_QUIT) {
                isRunning = false;
                continue;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            // メッセージがない時（暇な時）にゲームの更新と描画を行う

            // --- フレーム開始 ---
            GraphicsCore::Get().BeginFrame();

            // --- ゲームの更新と描画 ---
            // --- 描画ループ内 ---
			auto& gfx = GraphicsCore::Get();
            auto* ctx = &gfx.GetCommandContext();
            auto* psoState = ShaderManager::Get().GetPipelineState("Standard");

            // 1. PSOとルートシグネチャをセット
            ctx->SetPipelineState(psoState->PSO.Get());
            ctx->SetRootSignature(psoState->RootSignature.Get());

            // 2. ディスクリプタヒープをセット (以前作ったマネージャーを使用)
            ID3D12DescriptorHeap* ppHeaps[] = { gfx.GetSrvHeapManager().GetHeap() };
            ctx->GetCommandList()->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

            // 3. ルートパラメータに実際のデータをバインド
            // Root Parameter 0: カメラ定数バッファ (b0)
            //ctx->GetCommandList()->SetGraphicsRootConstantBufferView(0, cameraCB->GetGPUVirtualAddress());

            // Root Parameter 1: モデル定数バッファ (b1)
            //ctx->GetCommandList()->SetGraphicsRootConstantBufferView(1, modelCB->GetGPUVirtualAddress());

            // Root Parameter 2: テクスチャテーブル (t0)
            //ctx->GetCommandList()->SetGraphicsRootDescriptorTable(2, modelTexture->GetSrvHandle().GPUHandle);

            // --- エディタUIの構築と描画 ---
            editor.RenderUI();

            // --- フレーム終了（画面表示） ---
            GraphicsCore::Get().EndFrame();
        }
    }

    // クリーンアップ
    GraphicsCore::Get().FlushCommandQueue(); // GPUの処理完了を待つ
    editor.Shutdown();

    return 0;
}