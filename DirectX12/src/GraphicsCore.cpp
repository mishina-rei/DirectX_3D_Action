#include "DirectX12_pch.h"

#include "GraphicsCore.h"
#include <stdexcept>

constexpr int MAX_DESCRIPTOR = 1024;

// エラーチェック用マクロ（実際はエラーログ出力などに置き換えてください）
inline void ThrowIfFailed(HRESULT hr) {
    if (FAILED(hr)) {
        throw std::runtime_error("DirectX 12 API Error!");
    }
}

GraphicsCore::~GraphicsCore() {
    // 破棄する前にGPUの処理が終わるのを確実に待つ
    FlushCommandQueue();
    if (fenceEvent) {
        CloseHandle(fenceEvent);
    }
}

void GraphicsCore::Initialize(HWND hwnd, uint32_t width, uint32_t height) {
    // デバッグレイヤーの有効化（開発中のみ）
#if defined(_DEBUG)
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
        debugController->EnableDebugLayer();
    }
#endif

    // ファクトリとデバイスの作成
    ThrowIfFailed(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&dxgiFactory)));
    ThrowIfFailed(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device)));

    // コマンドキューの作成
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue)));


    // スワップチェーンの作成
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = width;
    swapChainDesc.Height = height;
    swapChainDesc.Format = GetBackBufferFormat();
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = frameCount;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapChain1;
    ThrowIfFailed(dxgiFactory->CreateSwapChainForHwnd(
        commandQueue.Get(), hwnd, &swapChainDesc, nullptr, nullptr, &swapChain1));
    ThrowIfFailed(swapChain1.As(&swapChain));

    frameIndex = swapChain->GetCurrentBackBufferIndex();

    // ディスクリプタヒープの作成
    // RTV(レンダーターゲット)用ヒープ
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = frameCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap)));

    // ディスクリプタヒープマネージャの初期化
    descriptorHeapManager.Initialize(device.Get(), MAX_DESCRIPTOR);

    // レンダーターゲットビュー(RTV)の作成
    SIZE_T rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());

    for (uint32_t i = 0; i < frameCount; i++) {
        ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i])));
        device->CreateRenderTargetView(renderTargets[i].Get(), nullptr, rtvHandle);
        rtvHandle.Offset(1, rtvDescriptorSize);
    }

    // コマンドアロケータの作成（フレームごとに個別のアロケータを用意）
    for (uint32_t i = 0; i < frameCount; ++i) {
        ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocators[i])));
    }

    // CommandContext を初期化
    commandContext.Initialize(device.Get(), commandQueue.Get(), commandAllocators[0].Get());

    // GPU同期用フェンスの作成
    ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
    fenceValue = 1;
    fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    // 配列の未初期化バグを防ぐため、ゼロクリアを明示的に行う
    for (uint32_t i = 0; i < frameCount; ++i) {
        frameFenceValues[i] = 0;
    }

    // DSV（深度バッファ）用ディスクリプタヒープの作成
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1; // 深度バッファは通常1つで十分
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // シェーダーから読まないのでNONE
    device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap));

    // 深度バッファ（テクスチャ）のリソースプロパティ設定
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT; // VRAM上に配置

    D3D12_RESOURCE_DESC depthDesc = {};
    depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthDesc.Alignment = 0;
    depthDesc.Width = width;
    depthDesc.Height = height;
    depthDesc.DepthOrArraySize = 1;
    depthDesc.MipLevels = 1;
    depthDesc.Format = GetDepthBufferFormat(); // DXGI_FORMAT_D32_FLOAT
    depthDesc.SampleDesc.Count = 1;
    depthDesc.SampleDesc.Quality = 0;
    depthDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; // 深度バッファとして使うフラグ

    // クリア値の最適化（1.0f = 最も奥 にクリアする設定）
    D3D12_CLEAR_VALUE optClear = {};
    optClear.Format = GetDepthBufferFormat();
    optClear.DepthStencil.Depth = 1.0f;
    optClear.DepthStencil.Stencil = 0;

    // リソースの作成
    // 初期状態は D3D12_RESOURCE_STATE_DEPTH_WRITE で作成
    device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &depthDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &optClear,
        IID_PPV_ARGS(&depthBuffer)
    );

    // DSVの作成 (ヒープに登録)
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = GetDepthBufferFormat();
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

    device->CreateDepthStencilView(
        depthBuffer.Get(),
        &dsvDesc,
        dsvHeap->GetCPUDescriptorHandleForHeapStart()
    );

    constantBufferPool.Initialize(device.Get(), 4 * 1024 * 1024);
}

void GraphicsCore::BeginFrame() {
    // CommandContext によるフレーム開始
    // 現在のフレームインデックスに対応するアロケータを渡す
    // GPUがそのアロケータをまだ使用中でないかを確認してからResetを呼ぶ
    if (fence && fence->GetCompletedValue() < frameFenceValues[frameIndex]) {
        // 指定フレームのフェンスが完了するまで待機
        ThrowIfFailed(fence->SetEventOnCompletion(frameFenceValues[frameIndex], fenceEvent));
        WaitForSingleObject(fenceEvent, INFINITE);
    }

    // GPUが完了した最新のフェンス値を渡し、使用済みのリングバッファ領域を解放する
    constantBufferPool.SyncCompletedFrames(fence->GetCompletedValue());

    commandContext.BeginFrame(commandAllocators[frameIndex].Get());

    // バックバッファを PRESENT -> RENDER_TARGET に遷移
    commandContext.TransitionResource(renderTargets[frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

    // 描画先(RTV)のセットと画面クリア
    SIZE_T rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, rtvDescriptorSize);
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(dsvHeap->GetCPUDescriptorHandleForHeapStart());
    commandContext.SetRenderTarget(&rtvHandle, &dsvHandle);

    const float clearColor[] = { 0.4f, 0.1f, 0.1f, 1.0f };
    commandContext.ClearColor(rtvHandle, clearColor);

	// 深度バッファのクリア
    commandContext.GetCommandList()->ClearDepthStencilView(
        dsvHandle,
        D3D12_CLEAR_FLAG_DEPTH,
        1.0f, // 1.0f (一番奥) でクリアするのが超重要！
        0,
        0,
        nullptr
    );
}

void GraphicsCore::EndFrame() {
    // バックバッファを RENDER_TARGET -> PRESENT に遷移
    commandContext.TransitionResource(renderTargets[frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

    // コマンドの記録終了と実行
    commandContext.EndFrame();

    // コマンドをGPUに送信した時点でフェンスをシグナルして、その値を現在のフレームに紐付ける
    // これにより次回同じフレームのアロケータを再利用する前にGPU完了を待てる
    const uint64_t currentFence = fenceValue;
    ThrowIfFailed(commandQueue->Signal(fence.Get(), currentFence));

    // このフレームでアロケートしたサイズと、それに紐づくフェンス値を記録する
    constantBufferPool.FinishFrame(currentFence);

    frameFenceValues[frameIndex] = currentFence;
    fenceValue++;

    HRESULT h = swapChain->Present(1, 0);
    HRESULT reason = device->GetDeviceRemovedReason();
    ThrowIfFailed(h);

    frameIndex = swapChain->GetCurrentBackBufferIndex();
}

void GraphicsCore::FlushCommandQueue() {
    // 現在のフェンス値をGPUにシグナルするように指示
    const uint64_t fenceToWaitFor = fenceValue;
    ThrowIfFailed(commandQueue->Signal(fence.Get(), fenceToWaitFor));
    fenceValue++;

    // GPUがシグナルを出すまでCPU側を待機させる
    if (fence->GetCompletedValue() < fenceToWaitFor) {
        ThrowIfFailed(fence->SetEventOnCompletion(fenceToWaitFor, fenceEvent));
        WaitForSingleObject(fenceEvent, INFINITE);
    }
}