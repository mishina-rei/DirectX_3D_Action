#pragma once
#include <d3d12.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <string>
#include <unordered_map>

using Microsoft::WRL::ComPtr;

// シェーダープログラムのバイナリを保持する構造体
struct ShaderProgram {
    ComPtr<ID3DBlob> VS;
    ComPtr<ID3DBlob> PS;
};

// パイプラインステートとルートシグネチャをまとめたもの
struct PipelineState {
    ComPtr<ID3D12RootSignature> RootSignature;
    ComPtr<ID3D12PipelineState> PSO;
};

class ShaderManager {
public:
    static ShaderManager& Get() {
        static ShaderManager instance;
        return instance;
    }

    void Initialize(ID3D12Device* _device);

    // HLSLファイルをコンパイルして読み込む
    void LoadShader(const std::string& name, const std::wstring& vsPath, const std::wstring& psPath);

    // 読み込んだシェーダーを使ってPSOを生成する（標準的な3Dモデル用）
    void CreateStandardPSO(const std::string& name, DXGI_FORMAT rtvFormat, DXGI_FORMAT dsvFormat);

    PipelineState* GetPipelineState(const std::string& name) {
        if (pipelines.count(name)) return &pipelines[name];
        return nullptr;
    }

private:
    ShaderManager() = default;

    ID3D12Device* device = nullptr;
    std::unordered_map<std::string, ShaderProgram> shaders;
    std::unordered_map<std::string, PipelineState> pipelines;

    ComPtr<ID3DBlob> CompileShader(const std::wstring& filePath, const char* entryPoint, const char* targetProfile);
};