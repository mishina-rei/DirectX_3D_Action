#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <vector>

using Microsoft::WRL::ComPtr;

class CommandContext {
public:
    CommandContext() = default;
    ~CommandContext() = default;

    // 初期化とフレーム管理
    void Initialize(ID3D12Device* device, ID3D12CommandQueue* commandQueue, ID3D12CommandAllocator* initAllocator);
    void BeginFrame(ID3D12CommandAllocator* allocator);
    void EndFrame(); // コマンドの記録を終了し、キューに積む

    // リソースの状態遷移（バリアを張る）
    void TransitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter);

    // レンダーターゲットのセットとクリア
    void SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE* rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE* dsvHandle);
    void ClearColor(D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, const float color[4]);
    void ClearDepth(D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle);

    // 描画関連
    void SetViewportAndScissor(uint32_t width, uint32_t height);    // ビューポートとシザー矩形の設定
    void SetRootSignature(ID3D12RootSignature* rootSig);            // ルートシグネチャの設定
    void SetPipelineState(ID3D12PipelineState* pso);                // パイプラインステートの設定
    void SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY topology);     // プリミティブトポロジーの設定
    void DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertex, uint32_t startInstance);     // インスタンス描画
    void DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndex, int32_t baseVertex, uint32_t startInstance);    // インデックス付きインスタンス描画

    // ゲッター（ImGuiのバックエンドに渡す用など）
    ID3D12GraphicsCommandList* GetCommandList() const { return commandList.Get(); }

private:
    ComPtr<ID3D12GraphicsCommandList> commandList;
    ID3D12CommandQueue* commandQueue = nullptr;

    // パフォーマンス最適化のため、バリアをある程度まとめて発行できるようにする
    std::vector<D3D12_RESOURCE_BARRIER> resourceBarriers;
    void FlushResourceBarriers();
};