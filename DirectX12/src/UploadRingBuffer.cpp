#include "DirectX12_pch.h"

#include "UploadRingBuffer.h"
#include <stdexcept>

UploadRingBuffer::~UploadRingBuffer() {
    if (buffer && mappedData) {
        buffer->Unmap(0, nullptr);
    }
}

void UploadRingBuffer::Initialize(ID3D12Device* device, size_t maxSize) {
    maxSize = AlignUp(maxSize); // 全体サイズも一応アライメント

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD; // CPUから書き込み可能

    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Width = maxSize;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    HRESULT hr = device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&buffer));

    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create Upload Ring Buffer.");
    }

    // UPLOADヒープは常にMapしたまま（Persistent Map）にしておくのが定石
    buffer->Map(0, nullptr, &mappedData);
    GPUBaseAddress = buffer->GetGPUVirtualAddress();
}

DynamicAllocation UploadRingBuffer::Allocate(size_t size) {
    std::lock_guard<std::mutex> lock(allocationMutex);

    size_t alignedSize = AlignUp(size);
    if (alignedSize > maxSize) {
        throw std::runtime_error("Allocation size exceeds ring buffer capacity.");
    }

    // リングバッファの終端に到達しそうな場合、先頭（0）に巻き戻せるかチェック
    if (head >= tail) {
        if (head + alignedSize > maxSize) {
            // 巻き戻し (Wrap Around)
            if (alignedSize > tail) {
                // 先頭に戻ってもTailにぶつかる場合はバッファ枯渇
                throw std::runtime_error("Ring buffer out of memory (Wait for GPU).");
            }
            // 巻き戻す際に、末尾の余った隙間サイズを現在のフレームの消費量に加算しておく
            currentFrameAllocatedSize += (maxSize - head);
            head = 0;
        }
    }
    else {
        // m_Head < m_Tail の状態（すでに巻き戻っている状態）
        if (head + alignedSize > tail) {
            throw std::runtime_error("Ring buffer out of memory (Wait for GPU).");
        }
    }

    DynamicAllocation alloc;
    alloc.CPUAddress = static_cast<uint8_t*>(mappedData) + head;
    alloc.GPUAddress = GPUBaseAddress + head;
    alloc.Size = alignedSize;

    head += alignedSize;
    currentFrameAllocatedSize += alignedSize;

    return alloc;
}

void UploadRingBuffer::FinishFrame(uint64_t currentFenceValue) {
    std::lock_guard<std::mutex> lock(allocationMutex);

    if (currentFrameAllocatedSize > 0) {
        inFlightFrames.push({ currentFenceValue, currentFrameAllocatedSize });
        currentFrameAllocatedSize = 0;
    }
}

void UploadRingBuffer::SyncCompletedFrames(uint64_t completedFenceValue) {
    std::lock_guard<std::mutex> lock(allocationMutex);

    // GPUが処理を完了したFence値以下のレコードをすべてキューから取り出し、Tailを進める
    while (!inFlightFrames.empty() && inFlightFrames.front().fenceValue <= completedFenceValue) {
        auto& record = inFlightFrames.front();

        tail += record.size;
        if (tail >= maxSize) {
            tail -= maxSize; // 巻き戻し計算
        }

        inFlightFrames.pop();
    }
}