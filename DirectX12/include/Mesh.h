#pragma once
#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class Mesh {
public:
    Mesh() = default;
    ~Mesh() = default;

    // 引数にコマンドリストを追加
    void Create(const void* vertexData, UINT vertexCount, UINT vertexStride,const void* indexData, UINT indexCount, DXGI_FORMAT indexFormat = DXGI_FORMAT_R32_UINT);

    // GPUへの転送が完了した後に呼んで、中間バッファのメモリを解放する
    void FreeUploadBuffers();

    const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const { return vbView; }
    const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const { return ibView; }
    UINT GetIndexCount() const { return indexCount; }

private:
    // 描画で実際に使われる、VRAM上の超高速バッファ
    ComPtr<ID3D12Resource> vertexBuffer;
    ComPtr<ID3D12Resource> indexBuffer;

    // GPUへのコピーが終わるまで一時的に保持しておく中間バッファ
    ComPtr<ID3D12Resource> vertexUploadBuffer;
    ComPtr<ID3D12Resource> indexUploadBuffer;

    D3D12_VERTEX_BUFFER_VIEW vbView = {};
    D3D12_INDEX_BUFFER_VIEW  ibView = {};

    UINT indexCount = 0;
};