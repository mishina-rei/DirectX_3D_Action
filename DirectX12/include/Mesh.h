#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <vector>
#include "CommandContext.h" // 以前作ったラッパー

class Mesh {
public:
    Mesh() = default;
    ~Mesh() = default;

    // 任意の頂点構造体を受け取れるようにテンプレート化しておくのが便利です
    template<typename T>
    void Initialize(
        ID3D12Device* device,
        CommandContext* initContext,
        const std::vector<T>& vertices,
        const std::vector<uint16_t>& indices);

    // 描画時に必要なゲッター
    const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const { return m_VertexBufferView; }
    const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const { return m_IndexBufferView; }
    uint32_t GetIndexCount() const { return m_IndexCount; }

private:
    // GPU専用のバッファ（描画中ずっと生き続ける）
    Microsoft::WRL::ComPtr<ID3D12Resource> m_VertexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_IndexBuffer;

    // 一時的なアップロード用バッファ（コピー完了後に破棄してよい）
    Microsoft::WRL::ComPtr<ID3D12Resource> m_UploadVertexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_UploadIndexBuffer;

    D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView = {};
    D3D12_INDEX_BUFFER_VIEW m_IndexBufferView = {};
    uint32_t m_IndexCount = 0;

    // バッファ作成のヘルパー
    Microsoft::WRL::ComPtr<ID3D12Resource> CreateBuffer(ID3D12Device* device, size_t size, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES state);
};

template<typename T>
void Mesh::Initialize(ID3D12Device* device, CommandContext* initContext, const std::vector<T>& vertices, const std::vector<uint16_t>& indices) {

    size_t vbSize = vertices.size() * sizeof(T);
    size_t ibSize = indices.size() * sizeof(uint16_t);
    m_IndexCount = static_cast<uint32_t>(indices.size());

    // GPU専用のDefaultヒープと、CPU用のUploadヒープを作成
    // Defaultヒープは最初はコピー先 (COPY_DEST) として作成します
    m_VertexBuffer = CreateBuffer(device, vbSize, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COPY_DEST);
    m_IndexBuffer = CreateBuffer(device, ibSize, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COPY_DEST);

    m_UploadVertexBuffer = CreateBuffer(device, vbSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
    m_UploadIndexBuffer = CreateBuffer(device, ibSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);

    // UploadヒープにCPUからデータを書き込む (Map -> memcpy -> Unmap)
    void* mappedData = nullptr;
    m_UploadVertexBuffer->Map(0, nullptr, &mappedData);
    memcpy(mappedData, vertices.data(), vbSize);
    m_UploadVertexBuffer->Unmap(0, nullptr);

    m_UploadIndexBuffer->Map(0, nullptr, &mappedData);
    memcpy(mappedData, indices.data(), ibSize);
    m_UploadIndexBuffer->Unmap(0, nullptr);

    // コマンドリストにコピー命令を積む
    auto cmdList = initContext->GetCommandList();
    cmdList->CopyBufferRegion(m_VertexBuffer.Get(), 0, m_UploadVertexBuffer.Get(), 0, vbSize);
    cmdList->CopyBufferRegion(m_IndexBuffer.Get(), 0, m_UploadIndexBuffer.Get(), 0, ibSize);

    // コピーが終わったら、リソースを「描画用」に状態遷移させる
    initContext->TransitionResource(m_VertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    initContext->TransitionResource(m_IndexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);

    // 描画時にAPIに渡す View (構造体) を作成
    m_VertexBufferView.BufferLocation = m_VertexBuffer->GetGPUVirtualAddress();
    m_VertexBufferView.StrideInBytes = sizeof(T);
    m_VertexBufferView.SizeInBytes = static_cast<UINT>(vbSize);

    m_IndexBufferView.BufferLocation = m_IndexBuffer->GetGPUVirtualAddress();
    m_IndexBufferView.Format = DXGI_FORMAT_R16_UINT; // uint16_t なので R16_UINT
    m_IndexBufferView.SizeInBytes = static_cast<UINT>(ibSize);
}