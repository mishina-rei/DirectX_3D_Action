#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <queue>
#include <mutex>
#include <stdexcept>

using Microsoft::WRL::ComPtr;

// CPUとGPUのハンドルをセットで扱うための構造体
struct DescriptorHandle {
    D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle = { 0 };
    D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle = { 0 };
    uint32_t Index = ~0u; // ヒープ内のインデックス
    bool IsValid() const { return Index != ~0u; }
};

class DescriptorHeapManager {
public:
    DescriptorHeapManager() = default;
    ~DescriptorHeapManager() = default;

    // ヒープの初期化 (デバイス、ヒープの最大数)
    void Initialize(ID3D12Device* device, uint32_t maxDescriptors);

    // 空いているディスクリプタを1つ確保する
    DescriptorHandle Allocate();

    // 不要になったディスクリプタを解放し、再利用可能にする
    void Free(const DescriptorHandle& handle);

    // インデックスからハンドルを取得する
    DescriptorHandle GetHandle(uint32_t index) const;

    // 生のヒープオブジェクトを取得（コマンドリストへのバインド用）
    ID3D12DescriptorHeap* GetHeap() const { return  heap.Get(); }

private:
    ComPtr<ID3D12DescriptorHeap>  heap;

    uint32_t  descriptorSize = 0; // ディスクリプタ1つあたりのバイトサイズ
    uint32_t  maxDescriptors = 0; // 最大数

    // アロケーション管理用
    std::queue<uint32_t>  freeIndices;
    std::mutex  allocationMutex; // スレッドセーフ用（非同期ロード対応）

    D3D12_CPU_DESCRIPTOR_HANDLE  cpuStart = { 0 };
    D3D12_GPU_DESCRIPTOR_HANDLE  gpuStart = { 0 };
};