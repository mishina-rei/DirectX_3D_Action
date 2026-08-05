#include "DirectX12_pch.h"

#include "Material.h"
#include "GraphicsCore.h"
#include "ShaderManager.h"

Material::Material(const std::string& VSname, const std::string& PSname) {

	ID3D12Device* device = GraphicsCore::Get().GetDevice();

	if (device == nullptr) {
		throw std::runtime_error("Material::Material: device is null");
	}
    IDxcBlob* vsBlob = ShaderManager::Get().GetShader(VSname);
    IDxcBlob* psBlob = ShaderManager::Get().GetShader(PSname);

    // シェーダーを解析してRoot Signatureと変数オフセットを取得
    metadata = ShaderReflection::Reflect(vsBlob,psBlob, device);

    // CPU側のバックバッファをリフレクションで得たサイズで確保
    cpuConstantBuffer.resize(metadata.cBufferSize, 0);

    // PSO生成用のキーを初期化
    PSOKey.rootSig = metadata.rootSignature.Get();
    PSOKey.isTransparent = false;
    PSOKey.disableCulling = false;
    PSOKey.rtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    PSOKey.dsvFormat = DXGI_FORMAT_D32_FLOAT;
    PSOKey.VS = vsBlob; 
    PSOKey.PS = psBlob;
}

void Material::SetData(const std::string& name, const void* data, uint32_t size) {
    auto it = metadata.variables.find(name);
    if (it != metadata.variables.end()) {
        const auto& varInfo = it->second;
        // 変数が見つかれば、CPUバッファの該当オフセットに直接 memcpy
        if (size <= varInfo.size) {
            memcpy(cpuConstantBuffer.data() + varInfo.offset, data, size);
        }
    }
}

void Material::SetFloat(const std::string& name, float value) {
    SetData(name, &value, sizeof(float));
}

void Material::SetVector(const std::string& name, DirectX::XMFLOAT4 value) {
    SetData(name, &value, sizeof(DirectX::XMFLOAT4));
}

void Material::SetMatrix(const std::string& name, const DirectX::XMMATRIX& value) {
    SetData(name, &value, sizeof(DirectX::XMMATRIX));
}

void Material::SetTexture(D3D12_GPU_DESCRIPTOR_HANDLE textureGpuHandle) {
    currentTexture = textureGpuHandle;
}

void Material::Bind() {

    ID3D12Device* device = GraphicsCore::Get().GetDevice();
	CommandContext* ctx = &GraphicsCore::Get().GetCommandContext();
	UploadRingBuffer* ringBuffer = &GraphicsCore::Get().GetConstantBufferPool();

    if (device == nullptr) {
        throw std::runtime_error("Material::Material: device is null");
    }
    // PSOの更新が必要ならキャッシュから取得
    if (isDirty) {
        currentPSO = PSOCache::Get().GetOrCreatePSO(device, PSOKey);
        isDirty = false;
    }

    // コマンドリストにPSOとRoot Signatureをセット
    auto cmd = ctx->GetCommandList();
    cmd->SetPipelineState(currentPSO);
    cmd->SetGraphicsRootSignature(metadata.rootSignature.Get());

    // 定数バッファの動的割り当てとコピー
    if (metadata.cBufferSize > 0) {
        // リングバッファから必要な分だけメモリ確保 (256バイトアライメントは自動)
        DynamicAllocation alloc = ringBuffer->Allocate(metadata.cBufferSize);

        // CPU側のデータをリングバッファ(GPUからも見えるメモリ)にコピー
        memcpy(alloc.CPUAddress, cpuConstantBuffer.data(), metadata.cBufferSize);

        // Root Signatureの該当パラメータ(b0など)にGPUアドレスをセット
        cmd->SetGraphicsRootConstantBufferView(metadata.cbRootParamIndex, alloc.GPUAddress);
    }

    // テクスチャのセット
    if (currentTexture.ptr != 0) {
        cmd->SetGraphicsRootDescriptorTable(metadata.texTableRootParamIndex, currentTexture);
    }
}