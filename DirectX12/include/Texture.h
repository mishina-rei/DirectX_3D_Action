#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <string>
#include "DescriptorHeapManager.h"

// DirectXTex.lib をリンクする
#if defined(_DEBUG)
#pragma comment(lib, "DirectXTex\\x64\\Debug\\DirectXTex.lib")
#else
#pragma comment(lib, "DirectXTex\\x64\\Release\\DirectXTex.lib")
#endif

class Texture {
public:
    Texture() = default;
    ~Texture();

    // 画像ファイルを読み込んでVRAMへ転送するコマンドを積む
    void CreateFromFile(const std::wstring& filePath);

    // 転送完了後に中間バッファを解放する
    void FreeUploadBuffer();

    // Material::SetTexture に渡すためのGPUハンドルを取得
    D3D12_GPU_DESCRIPTOR_HANDLE GetSrvGpuHandle() const { return srvHandle.GPUHandle; }

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> textureBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer;

    DescriptorHandle srvHandle; // SRVのディスクリプタ
};