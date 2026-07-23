#include <windows.h>

#include "DirectX12.h"

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

struct Vertex {
    DirectX::XMFLOAT3 Pos;
    DirectX::XMFLOAT3 Normal;
    DirectX::XMFLOAT2 UV;
};

// 立方体の8頂点
std::vector<Vertex> vertices = {
    // Pos, Normal, UV の順
    Vertex{ {-1.0f, -1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f} },
    Vertex{ {-1.0f,  1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f} },
    Vertex{ { 1.0f,  1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f} },
    Vertex{ { 1.0f, -1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f} },
    Vertex{ {-1.0f, -1.0f,  1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f} },
    Vertex{ {-1.0f,  1.0f,  1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f} },
    Vertex{ { 1.0f,  1.0f,  1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f} },
    Vertex{ { 1.0f, -1.0f,  1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f} }
};

// 描画順（時計回りが表）
std::vector<uint16_t> indices = {
    0, 1, 2, 0, 2, 3, // 前
    4, 6, 5, 4, 7, 6, // 後
    4, 5, 1, 4, 1, 0, // 左
    3, 2, 6, 3, 6, 7, // 右
    1, 5, 6, 1, 6, 2, // 上
    4, 0, 3, 4, 3, 7  // 下
};

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
    //ShaderManager::Get().CreateStandardPSO("Standard", GraphicsCore::Get().GetBackBufferFormat(), GraphicsCore::Get().GetDepthBufferFormat());

    // マテリアル
    auto cubeMaterial = std::make_shared<Material>("Standard");

    auto& gfx = GraphicsCore::Get();
    auto ictx = gfx.GetCommandContext();

    ictx.BeginFrame(gfx.GetCurrentCommandAllocator());

    // 内部でAssimpがパースし、ctx に対して CopyBufferRegion 命令を積みます
	Mesh mesh = Mesh();
    mesh.Initialize<Vertex>(gfx.GetDevice(),&ictx,vertices, indices);

    ictx.EndFrame();

    gfx.FlushCommandQueue();

    // ImGui用の一時変数
    DirectX::XMFLOAT4 cubeColor = { 0.2f, 0.6f, 0.9f, 1.0f };
    float rotationAngle = 0.0f;

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
            //auto* psoState = ShaderManager::Get().GetPipelineState("Standard");

            //// 1. PSOとルートシグネチャをセット
            //ctx->SetPipelineState(psoState->PSO.Get());
            //ctx->SetRootSignature(psoState->RootSignature.Get());

            
            // 1. ビュー行列（カメラの位置と向き）
            DirectX::XMVECTOR eye = DirectX::XMVectorSet(0.0f, 2.0f, -5.0f, 0.0f); // カメラをZ軸の手前(-5)、少し上(2)に配置
            DirectX::XMVECTOR target = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f); // キューブの中心（原点）を見る
            DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f); // 上はY方向
            DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(eye, target, up);

            // 2. プロジェクション行列（遠近感と画角）
            float fov = DirectX::XMConvertToRadians(45.0f);
            float aspect = 1920.0f / 1080.0f; // ウィンドウサイズに合わせる
            DirectX::XMMATRIX proj = DirectX::XMMatrixPerspectiveFovLH(fov, aspect, 0.1f, 100.0f);

            // 3. 掛け合わせて ViewProjection にする
            auto viewProj = DirectX::XMMatrixMultiply(view, proj);
            auto world = DirectX::XMMatrixRotationY(DirectX::XMConvertToRadians(rotationAngle));

            //auto viewProj = DirectX::XMMatrixIdentity(); // 仮のビュー射影行列   
            //auto world = DirectX::XMMatrixRotationY(DirectX::XMConvertToRadians(rotationAngle));
            auto wvp = DirectX::XMMatrixMultiply(world, viewProj);

            auto viewProjTransposed = DirectX::XMMatrixTranspose(viewProj);
            auto worldTransposed = DirectX::XMMatrixTranspose(world);

            // 3. マテリアルへのデータセット（名前ベース！）
            cubeMaterial->SetMatrix("viewProjection", viewProj);
            cubeMaterial->SetMatrix("world", world);

            // ディスクリプタヒープをセット
            ID3D12DescriptorHeap* ppHeaps[] = { gfx.GetSrvHeapManager().GetHeap() };
            ctx->GetCommandList()->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

            // マテリアルをバインド（裏側でリングバッファの確保とmemcpy、PSOセットが走る）
            cubeMaterial->Bind();

            // 描画範囲（ビューポート）と切り抜き範囲（シザー矩形）を画面サイズに設定
            D3D12_VIEWPORT viewport = { 0.0f, 0.0f, 1920.0f, 1080.0f, 0.0f, 1.0f };
            D3D12_RECT scissorRect = { 0, 0, 1920, 1080 };

            ctx->GetCommandList()->RSSetViewports(1, &viewport);
            ctx->GetCommandList()->RSSetScissorRects(1, &scissorRect);

            // 頂点・インデックスバッファのセット
            ctx->GetCommandList()->IASetVertexBuffers(0, 1, &mesh.GetVertexBufferView());
            ctx->GetCommandList()->IASetIndexBuffer(&mesh.GetIndexBufferView());
            ctx->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = gfx.GetRtvHandle();
            D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = gfx.GetDsvHandle();

            ctx->GetCommandList()->DrawIndexedInstanced(mesh.GetIndexCount(), 1, 0, 0, 0);



			editor.BeginUI(); // ImGuiのフレーム開始
            // 1. ImGuiのUI構築
            ImGui::Begin("Inspector");
            ImGui::ColorEdit4("Cube Color", &cubeColor.x); // カラーピッカー！
            ImGui::SliderFloat("Rotation", &rotationAngle, 0.0f, 360.0f);
            ImGui::End();


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

