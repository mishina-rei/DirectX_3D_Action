#include "DirectX12_pch.h"
#include "Mesh.h"
#include "GraphicsCore.h"

void Mesh::Create(
    const void* vertexData, UINT vCount, UINT vStride,
    const void* indexData, UINT iCount, DXGI_FORMAT iFormat)
{
	ID3D12Device* device = GraphicsCore::Get().GetDevice();
	ID3D12GraphicsCommandList* cmdList = GraphicsCore::Get().GetCommandList();

	if (device == nullptr || cmdList == nullptr) {
		throw std::runtime_error("Mesh::Create: device or cmdList is null");
	}

    this->indexCount = iCount;

    // ヒープ設定（VRAM用とCPU-GPU転送用）
    D3D12_HEAP_PROPERTIES defaultHeapProp = {};
    defaultHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_HEAP_PROPERTIES uploadHeapProp = {};
    uploadHeapProp.Type = D3D12_HEAP_TYPE_UPLOAD;

    // -----------------------------------------------------
    // 頂点バッファの作成
    // -----------------------------------------------------
    UINT vbSize = vCount * vStride;

    D3D12_RESOURCE_DESC vbDesc = {};
    vbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    vbDesc.Width = vbSize;
    vbDesc.Height = 1;
    vbDesc.DepthOrArraySize = 1;
    vbDesc.MipLevels = 1;
    vbDesc.Format = DXGI_FORMAT_UNKNOWN;
    vbDesc.SampleDesc.Count = 1;
    vbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    vbDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    // DEFAULTヒープに作成（最初はコピー先なので COPY_DEST にする）
    device->CreateCommittedResource(
        &defaultHeapProp, D3D12_HEAP_FLAG_NONE, &vbDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&vertexBuffer));

    // UPLOADヒープに作成（中間バッファ）
    device->CreateCommittedResource(
        &uploadHeapProp, D3D12_HEAP_FLAG_NONE, &vbDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexUploadBuffer));

    // UPLOADヒープにCPUからデータを書き込む
    void* mappedVertexData = nullptr;
    vertexUploadBuffer->Map(0, nullptr, &mappedVertexData);
    memcpy(mappedVertexData, vertexData, vbSize);
    vertexUploadBuffer->Unmap(0, nullptr);

    // コマンドリストにコピー命令を積む
    cmdList->CopyBufferRegion(vertexBuffer.Get(), 0, vertexUploadBuffer.Get(), 0, vbSize);

    // コピーが終わったら、状態を「コピー先」から「頂点バッファとして読み取り」へ変更（バリア）
    D3D12_RESOURCE_BARRIER vbBarrier = {};
    vbBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    vbBarrier.Transition.pResource = vertexBuffer.Get();
    vbBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    vbBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    cmdList->ResourceBarrier(1, &vbBarrier);

    // ビューの設定
    vbView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
    vbView.SizeInBytes = vbSize;
    vbView.StrideInBytes = vStride;

    // -----------------------------------------------------
    // インデックスバッファの作成
    // -----------------------------------------------------
    UINT indexStride = (iFormat == DXGI_FORMAT_R16_UINT) ? 2 : 4;
    UINT ibSize = iCount * indexStride;

    D3D12_RESOURCE_DESC ibDesc = vbDesc;
    ibDesc.Width = ibSize;

    // DEFAULTヒープに作成
    device->CreateCommittedResource(
        &defaultHeapProp, D3D12_HEAP_FLAG_NONE, &ibDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&indexBuffer));

    // UPLOADヒープに作成
    device->CreateCommittedResource(
        &uploadHeapProp, D3D12_HEAP_FLAG_NONE, &ibDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&indexUploadBuffer));

    // UPLOADヒープにCPUからデータを書き込む
    void* mappedIndexData = nullptr;
    indexUploadBuffer->Map(0, nullptr, &mappedIndexData);
    memcpy(mappedIndexData, indexData, ibSize);
    indexUploadBuffer->Unmap(0, nullptr);

    // コマンドリストにコピー命令を積む
    cmdList->CopyBufferRegion(indexBuffer.Get(), 0, indexUploadBuffer.Get(), 0, ibSize);

    // 状態を「コピー先」から「インデックスバッファとして読み取り」へ変更（バリア）
    D3D12_RESOURCE_BARRIER ibBarrier = {};
    ibBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    ibBarrier.Transition.pResource = indexBuffer.Get();
    ibBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    ibBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_INDEX_BUFFER;
    cmdList->ResourceBarrier(1, &ibBarrier);

    // ビューの設定
    ibView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
    ibView.SizeInBytes = ibSize;
    ibView.Format = iFormat;
}

// 転送が終わったら不要なメモリを解放する
void Mesh::FreeUploadBuffers() {
    vertexUploadBuffer.Reset();
    indexUploadBuffer.Reset();
}