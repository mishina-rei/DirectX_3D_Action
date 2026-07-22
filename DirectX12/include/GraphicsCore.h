#pragma once
#include "DirectX12_pch.h"
#include "DescriptorHeapManager.h"
#include "CommandContext.h"

using Microsoft::WRL::ComPtr;

class GraphicsCore {
public:
    static GraphicsCore& Get() {
        static GraphicsCore instance;
        return instance;
    }

    void Initialize(HWND hwnd, uint32_t width, uint32_t height);
    void BeginFrame();
    void EndFrame();
    void FlushCommandQueue();

    // ゲッター群
    ID3D12Device* GetDevice() const { return  device.Get(); }
    ID3D12GraphicsCommandList* GetCommandList() const { return commandContext.GetCommandList(); }
    // CommandContext へアクセス
    CommandContext& GetCommandContext() { return commandContext; }
    DescriptorHeapManager& GetSrvHeapManager() { return descriptorHeapManager; }
    DXGI_FORMAT GetBackBufferFormat() const { return DXGI_FORMAT_R8G8B8A8_UNORM; }
    ID3D12CommandQueue* GetCommandQueue()const { return commandQueue.Get(); }

    // 深度バッファのフォーマットを取得（32bit Float を推奨）
    DXGI_FORMAT GetDepthBufferFormat() const { return DXGI_FORMAT_D32_FLOAT; }

    // コマンドリストにセットするためのDSVハンドルを取得
    D3D12_CPU_DESCRIPTOR_HANDLE GetDsvHandle() const;

private:
    GraphicsCore() = default;
    ~GraphicsCore();

    // フレーム同期
    ComPtr<ID3D12Fence>  fence;
    uint64_t  fenceValue = 0;
    HANDLE  fenceEvent = nullptr;

    static const uint8_t frameCount = 2;
    uint32_t  frameIndex = 0;
    ComPtr<ID3D12Resource>  renderTargets[frameCount];

    // DX12コアオブジェクト
    ComPtr<IDXGIFactory7>  dxgiFactory;
    ComPtr<ID3D12Device>  device;
    ComPtr<ID3D12CommandQueue>  commandQueue;
    ComPtr<IDXGISwapChain4>  swapChain;
    // フレームごとにコマンドアロケータを用意して、GPUが使用中のアロケータをリセットしないようにする
    ComPtr<ID3D12CommandAllocator>  commandAllocators[frameCount];
    // フレームごとのフェンス値（コマンドアロケータ再利用の同期に使用）
    uint64_t frameFenceValues[frameCount] = {};
    ComPtr<ID3D12GraphicsCommandList>  commandList;

    // コマンド操作を集約するためのラッパークラス
    CommandContext commandContext;

    // ディスクリプタヒープ
    ComPtr<ID3D12DescriptorHeap>  rtvHeap;
    DescriptorHeapManager descriptorHeapManager;

    // 深度バッファ用リソース
    ComPtr<ID3D12Resource> depthBuffer;
    ComPtr<ID3D12DescriptorHeap> dsvHeap;
};