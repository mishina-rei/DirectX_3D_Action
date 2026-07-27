#include "DirectX12_pch.h"
#include "Texture.h"
#include "GraphicsCore.h"
#include <DirectXTex/DirectXTex.h> // DirectXTex をインクルード
#include <stdexcept>


Texture::~Texture() {
    if (srvHandle.IsValid()) {
        GraphicsCore::Get().GetSrvHeapManager().Free(srvHandle);
    }
}

void Texture::CreateFromFile(const std::wstring& filePath) {
    ID3D12Device* device = GraphicsCore::Get().GetDevice();
    ID3D12GraphicsCommandList* cmdList = GraphicsCore::Get().GetCommandList();
    DirectX::TexMetadata metadata;
    DirectX::ScratchImage scratchImage;
    HRESULT hr = S_OK;

    // 拡張子によってDDSとそれ以外(PNG, JPG等)を読み分ける
    if (filePath.find(L".dds") != std::wstring::npos || filePath.find(L".DDS") != std::wstring::npos) {
        hr = DirectX::LoadFromDDSFile(filePath.c_str(), DirectX::DDS_FLAGS_NONE, &metadata, scratchImage);
    }
    else {
        hr = DirectX::LoadFromWICFile(filePath.c_str(), DirectX::WIC_FLAGS_NONE, &metadata, scratchImage);

        // ミップマップ
        // DirectX::ScratchImage mipChain;
        // DirectX::GenerateMipMaps(scratchImage.GetImages(), scratchImage.GetImageCount(), scratchImage.GetMetadata(), DirectX::TEX_FILTER_DEFAULT, 0, mipChain);
        // scratchImage = std::move(mipChain);
        // metadata = scratchImage.GetMetadata();
    }

    if (FAILED(hr)) {
        throw std::runtime_error("Failed to load texture.");
    }

    // DEFAULTヒープにテクスチャリソースを作成 (DirectXTexのヘルパーを使用)
    hr = DirectX::CreateTexture(device, metadata, &textureBuffer);
    if (FAILED(hr)) throw std::runtime_error("Failed to create texture resource.");

    // アップロード用データの準備
    std::vector<D3D12_SUBRESOURCE_DATA> subresources;
    hr = DirectX::PrepareUpload(device, scratchImage.GetImages(), scratchImage.GetImageCount(), metadata, subresources);
    if (FAILED(hr)) throw std::runtime_error("Failed to prepare texture upload.");

    // UPLOADヒープの作成
    const UINT64 uploadBufferSize = GetRequiredIntermediateSize(textureBuffer.Get(), 0, static_cast<UINT>(subresources.size()));

    D3D12_HEAP_PROPERTIES uploadHeapProp = {};
    uploadHeapProp.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC uploadDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

    hr = device->CreateCommittedResource(
        &uploadHeapProp, D3D12_HEAP_FLAG_NONE, &uploadDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuffer));
    if (FAILED(hr)) throw std::runtime_error("Failed to create upload buffer.");

    // データのコピー (d3dx12.h の関数を使用)
    UpdateSubresources(cmdList, textureBuffer.Get(), uploadBuffer.Get(), 0, 0, static_cast<UINT>(subresources.size()), subresources.data());

    // 状態を「コピー先」から「ピクセルシェーダーリソース」へ変更（バリア）
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = textureBuffer.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    cmdList->ResourceBarrier(1, &barrier);

    // SRVの作成
    srvHandle = GraphicsCore::Get().GetSrvHeapManager().Allocate();

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = metadata.format;

    // 次元（2D, 3D, Cube）をメタデータから自動判定
    if (metadata.IsCubemap()) {
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    }
    else if (metadata.dimension == DirectX::TEX_DIMENSION_TEXTURE3D) {
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
    }
    else {
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    }
    srvDesc.Texture2D.MipLevels = static_cast<UINT>(metadata.mipLevels);

    device->CreateShaderResourceView(textureBuffer.Get(), &srvDesc, srvHandle.CPUHandle);
}

void Texture::FreeUploadBuffer() {
    uploadBuffer.Reset();
}