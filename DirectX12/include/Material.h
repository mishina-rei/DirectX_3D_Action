#pragma once
#include "ShaderReflection.h"
#include "PSOCache.h"
#include "UploadRingBuffer.h"
#include "CommandContext.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <DirectXMath.h>

class Material {
public:
    Material(const std::string& VSname, const std::string& PSname);

    // --- 状態の変更 ---
    void SetTransparent(bool isTransparent) { PSOKey.isTransparent = isTransparent; isDirty = true; }
    void SetCullingDisabled(bool disable) { PSOKey.disableCulling = disable; isDirty = true; }

    // --- パラメータのセット（リフレクションを利用） ---
    void SetFloat(const std::string& name, float value);
    void SetVector(const std::string& name, DirectX::XMFLOAT4 value);
    void SetMatrix(const std::string& name, const DirectX::XMMATRIX& value);

    // 内部データ書き込みヘルパー
    void SetData(const std::string& name, const void* data, uint32_t size);

    // テクスチャのセット (GPUハンドルを渡す)
    void SetTexture(D3D12_GPU_DESCRIPTOR_HANDLE textureGpuHandle);

    // --- 描画時のバインド処理 ---
    void Bind();

private:
    ShaderMetadata metadata;
    PSOKey PSOKey;
    bool isDirty = true;
    ID3D12PipelineState* currentPSO = nullptr;

    // 定数バッファの内容を一時的に保持するCPU側メモリ
    std::vector<uint8_t> cpuConstantBuffer;

    // バインド予定のテクスチャ
    D3D12_GPU_DESCRIPTOR_HANDLE currentTexture = { 0 };

};