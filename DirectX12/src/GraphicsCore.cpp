#include "DirectX12_pch.h"

#include "GraphicsCore.h"
#include <stdexcept>

// エラーチェック用マクロ（実際はエラーログ出力などに置き換えてください）
inline void ThrowIfFailed(HRESULT hr) {
    if (FAILED(hr)) {
        throw std::runtime_error("DirectX 12 API Error!");
    }
}

GraphicsCore::~GraphicsCore() {
    // 破棄する前にGPUの処理が終わるのを確実に待つ
    FlushCommandQueue();
    if ( fenceEvent) {
        CloseHandle( fenceEvent);
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
    ThrowIfFailed(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(& dxgiFactory)));
    ThrowIfFailed(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(& device)));

    // コマンドキューの作成
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    ThrowIfFailed( device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(& commandQueue)));

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
    ThrowIfFailed( dxgiFactory->CreateSwapChainForHwnd(
         commandQueue.Get(), hwnd, &swapChainDesc, nullptr, nullptr, &swapChain1));
    ThrowIfFailed(swapChain1.As(& swapChain));

     frameIndex =  swapChain->GetCurrentBackBufferIndex();

    // ディスクリプタヒープの作成
    // RTV(レンダーターゲット)用ヒープ
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = frameCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed( device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(& rtvHeap)));

    // SRV(シェーダーリソースビュー)用ヒープ: ImGuiのフォントやゲーム内のテクスチャ用
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.NumDescriptors = 1024; // 余裕を持って1024個確保
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; // 重要: シェーダーから読めるようにする
    ThrowIfFailed( device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(& srvHeap)));

    // ディスクリプタヒープマネージャの初期化
    //descriptorHeapManager.Initialize(device.Get(), srvHeapDesc.NumDescriptors);

    // レンダーターゲットビュー(RTV)の作成
    SIZE_T rtvDescriptorSize =  device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle( rtvHeap->GetCPUDescriptorHandleForHeapStart());

    for (uint32_t i = 0; i < frameCount; i++) {
        ThrowIfFailed( swapChain->GetBuffer(i, IID_PPV_ARGS(& renderTargets[i])));
         device->CreateRenderTargetView( renderTargets[i].Get(), nullptr, rtvHandle);
        rtvHandle.Offset(1, rtvDescriptorSize);
    }

    // コマンドアロケータとリストの作成
    ThrowIfFailed( device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(& commandAllocator)));
    ThrowIfFailed( device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,  commandAllocator.Get(), nullptr, IID_PPV_ARGS(& commandList)));

    // コマンドリストは最初はClose状態にしておく（BeginFrameでResetするため）
     commandList->Close();

    // GPU同期用フェンスの作成
    ThrowIfFailed( device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(& fence)));
     fenceValue = 1;
     fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

void GraphicsCore::BeginFrame() {
    // コマンドアロケータとリストのクリア（リセット）
    ThrowIfFailed( commandAllocator->Reset());
    ThrowIfFailed( commandList->Reset( commandAllocator.Get(), nullptr));

    // バックバッファを「画面表示用(PRESENT)」から「描画用(RENDER_TARGET)」へ状態遷移
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
         renderTargets[ frameIndex].Get(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET);
     commandList->ResourceBarrier(1, &barrier);

    // 描画先(RTV)のセットと画面クリア
    SIZE_T rtvDescriptorSize =  device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle( rtvHeap->GetCPUDescriptorHandleForHeapStart(),  frameIndex, rtvDescriptorSize);

     commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    // 背景をダークグレーでクリア
    const float clearColor[] = { 0.1f, 0.1f, 0.1f, 1.0f };
     commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

    // ImGui描画のためにSRVヒープをコマンドリストにセット
    //ID3D12DescriptorHeap* descriptorHeaps[] = {  srvHeap.Get() };
    // commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
}

void GraphicsCore::EndFrame() {
    // バックバッファを「描画用(RENDER_TARGET)」から「画面表示用(PRESENT)」へ状態遷移
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
         renderTargets[ frameIndex].Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT);
     commandList->ResourceBarrier(1, &barrier);

    // コマンドリストの記録終了
    ThrowIfFailed( commandList->Close());

    // コマンドキューに積んで実行
    ID3D12CommandList* ppCommandLists[] = {  commandList.Get() };
    commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
    
    // 次のフレームの準備のためにGPUの実行完了を待つ
    // ※本格的なエンジンでは、ここで待たずに次のバックバッファのアロケータを使うように
    // フレーム単位でフェンスを管理しますが、今回はシンプルにするため毎回Flushしています。
    FlushCommandQueue();

    HRESULT h = swapChain->Present(1, 0);
    HRESULT reason = device->GetDeviceRemovedReason();
    // 画面のフリップ（表示）
    ThrowIfFailed(h);

    frameIndex =  swapChain->GetCurrentBackBufferIndex();
}

void GraphicsCore::FlushCommandQueue() {
    // 現在のフェンス値をGPUにシグナルするように指示
    const uint64_t fenceToWaitFor =  fenceValue;
    ThrowIfFailed( commandQueue->Signal( fence.Get(), fenceToWaitFor));
     fenceValue++;

    // GPUがシグナルを出すまでCPU側を待機させる
    if ( fence->GetCompletedValue() < fenceToWaitFor) {
        ThrowIfFailed( fence->SetEventOnCompletion(fenceToWaitFor,  fenceEvent));
        WaitForSingleObject( fenceEvent, INFINITE);
    }
}