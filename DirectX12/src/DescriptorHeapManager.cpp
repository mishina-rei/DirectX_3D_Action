#include "DirectX12_pch.h"
#include "DescriptorHeapManager.h"

void DescriptorHeapManager::Initialize(ID3D12Device* device, uint32_t maxDescriptors) {
    this->maxDescriptors = maxDescriptors;

    // CBV/SRV/UAV 用のヒープを作成。シェーダーから参照可能にするため D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE を指定。
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors =  maxDescriptors;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    heapDesc.NodeMask = 0;

    HRESULT hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(& heap));
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create SRV Descriptor Heap.");
    }

    // デバイスごとのディスクリプタのサイズを取得（環境によってサイズが異なるため必須）
     descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // ヒープの先頭アドレスを取得
     cpuStart =  heap->GetCPUDescriptorHandleForHeapStart();
     gpuStart =  heap->GetGPUDescriptorHandleForHeapStart();

    // 最初はすべてのインデックスが空いている状態
    for (uint32_t i = 0; i <  maxDescriptors; ++i) {
         freeIndices.push(i);
    }
}

DescriptorHandle DescriptorHeapManager::Allocate() {
    std::lock_guard<std::mutex> lock( allocationMutex);

    if ( freeIndices.empty()) {
        throw std::runtime_error("SRV Descriptor Heap is full!");
    }

    // 空きインデックスをキューから取り出す
    uint32_t index =  freeIndices.front();
     freeIndices.pop();

    // インデックスからCPU/GPUのアドレスを計算する
    DescriptorHandle handle;
    handle.Index = index;

    handle.CPUHandle.ptr =  cpuStart.ptr + (index *  descriptorSize);
    handle.GPUHandle.ptr =  gpuStart.ptr + (index *  descriptorSize);

    return handle;
}

void DescriptorHeapManager::Free(const DescriptorHandle& handle) {
    if (!handle.IsValid()) return;

    std::lock_guard<std::mutex> lock( allocationMutex);

    // 解放されたインデックスをキューに戻し、再利用可能にする
     freeIndices.push(handle.Index);
}

DescriptorHandle DescriptorHeapManager::GetHandle(uint32_t index) const {
    if (index >= maxDescriptors) {
        throw std::out_of_range("Index out of range for descriptor heap.");
    }

    DescriptorHandle handle;
    handle.Index = index;
    handle.CPUHandle.ptr = cpuStart.ptr + (index * descriptorSize);
    handle.GPUHandle.ptr = gpuStart.ptr + (index * descriptorSize);
    return handle;
}