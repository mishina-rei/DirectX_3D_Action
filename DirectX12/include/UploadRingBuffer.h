#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <queue>
#include <mutex>

using Microsoft::WRL::ComPtr;

// 確保したメモリのアドレス情報
struct DynamicAllocation {
    void* CPUAddress = nullptr;
    D3D12_GPU_VIRTUAL_ADDRESS GPUAddress = 0;
    size_t Size = 0;
};

class UploadRingBuffer {
public:
    UploadRingBuffer() = default;
    ~UploadRingBuffer();

    void Initialize(ID3D12Device* device, size_t maxSize);

    // 必要なサイズのメモリを確保する（自動で256バイトアライメントされる）
    DynamicAllocation Allocate(size_t size);

    // フレームの終わりに呼び出し、ここまで使ったメモリとFence値を紐づける
    void FinishFrame(uint64_t currentFenceValue);

    // GPUが処理を終えたFence値を受け取り、不要になったメモリ（Tail）を解放する
    void SyncCompletedFrames(uint64_t completedFenceValue);

private:
    ComPtr<ID3D12Resource> buffer;
    void* mappedData = nullptr;
    D3D12_GPU_VIRTUAL_ADDRESS GPUBaseAddress = 0;

    size_t maxSize = 0;
    size_t head = 0; // CPUが次に書き込む位置
    size_t tail = 0; // GPUが読み込み終わった位置

    // フレームごとの確保サイズとFence値を記録するキュー
    struct FrameRecord {
        uint64_t fenceValue;
        size_t size;
    };
    std::queue<FrameRecord> inFlightFrames;
    size_t currentFrameAllocatedSize = 0;

    std::mutex allocationMutex;

    // 256バイトアライメント計算用のヘルパー
    static size_t AlignUp(size_t size, size_t alignment = 256) {
        return (size + alignment - 1) & ~(alignment - 1);
    }
};