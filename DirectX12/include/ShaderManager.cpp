#include "DirectX12_pch.h"

#include "ShaderManager.h"
#include <stdexcept>

void ShaderManager::Initialize(ID3D12Device* _device) {
    device = _device;
}

ComPtr<ID3DBlob> ShaderManager::CompileShader(const std::wstring& filePath, const char* entryPoint, const char* targetProfile) {
    ComPtr<ID3DBlob> byteCode = nullptr;
    ComPtr<ID3DBlob> errors = nullptr;

    UINT compileFlags = 0;
#if defined(_DEBUG)
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    HRESULT hr = D3DCompileFromFile(filePath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entryPoint, targetProfile, compileFlags, 0, &byteCode, &errors);

    if (FAILED(hr)) {
        // コンパイルエラー（文法ミスなど）がある場合
        if (errors) {
            std::string errMsg = (char*)errors->GetBufferPointer();
            OutputDebugStringA(errMsg.c_str());
            throw std::runtime_error("Shader Compile Error: " + errMsg);
        }
        // errorsが空の場合（ファイル自体が見つからない場合など）
        else {
            // filePathを文字列に変換して表示
            std::string fileStr(filePath.begin(), filePath.end());
            throw std::runtime_error("Failed to compile shader. File not found: " + fileStr);
        }
    }
    return byteCode;
}

void ShaderManager::LoadShader(const std::string& name, const std::wstring& vsPath, const std::wstring& psPath) {

    if (shaders.count(name)) return;

    ShaderProgram program;

    // VSファイルから main をコンパイル
    program.VS = CompileShader(vsPath, "main", "vs_5_1");

    // PSファイルから main をコンパイル
    program.PS = CompileShader(psPath, "main", "ps_5_1");

    shaders[name] = program;
}

void ShaderManager::LoadPSShader(const std::string& name, const std::wstring& psPath) {
    if (shaders.count(name)) return;

    ShaderProgram program;
    program.PS = CompileShader(psPath, "main", "ps_5_1");
    shaders[name] = program;
}

void ShaderManager::LoadVSShader(const std::string& name, const std::wstring& vsPath) {
    if (shaders.count(name)) return;

    ShaderProgram program;
    program.VS = CompileShader(vsPath, "main", "vs_5_1");
    shaders[name] = program;
}

void ShaderManager::CreateStandardPSO(const std::string& name, DXGI_FORMAT rtvFormat, DXGI_FORMAT dsvFormat) {
    auto& shader = shaders[name];
    PipelineState pipeline;

    // Root Signatureの作成
    D3D12_ROOT_PARAMETER rootParameters[3];

    // パラメータ0: cbv(b0) カメラ情報
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[0].Descriptor.ShaderRegister = 0; // b0
    rootParameters[0].Descriptor.RegisterSpace = 0;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // パラメータ1: cbv(b1) モデル情報
    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[1].Descriptor.ShaderRegister = 1; // b1
    rootParameters[1].Descriptor.RegisterSpace = 0;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // パラメータ2: Descriptor Table (テクスチャ t0)
    D3D12_DESCRIPTOR_RANGE srvRange;
    srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange.NumDescriptors = 1;
    srvRange.BaseShaderRegister = 0; // t0
    srvRange.RegisterSpace = 0;
    srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
    rootParameters[2].DescriptorTable.pDescriptorRanges = &srvRange;
    rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // サンプラーの設定 (s0)
    D3D12_STATIC_SAMPLER_DESC sampler = {};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.MipLODBias = 0;
    sampler.MaxAnisotropy = 0;
    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    sampler.MinLOD = 0.0f;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;
    sampler.ShaderRegister = 0; // s0
    sampler.RegisterSpace = 0;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.NumParameters = _countof(rootParameters);
    rootSigDesc.pParameters = rootParameters;
    rootSigDesc.NumStaticSamplers = 1;
    rootSigDesc.pStaticSamplers = &sampler;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
    device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&pipeline.RootSignature));

    // ---------------------------------------------------------
    // PSO (Pipeline State Object) の作成
    // ---------------------------------------------------------
    // 頂点レイアウトの定義 (Vertex構造体と一致させる)
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.pRootSignature = pipeline.RootSignature.Get();
    psoDesc.VS = { shader.VS->GetBufferPointer(), shader.VS->GetBufferSize() };
    psoDesc.PS = { shader.PS->GetBufferPointer(), shader.PS->GetBufferSize() };

    // ラスタライザーステート (カリングなど)
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
    psoDesc.RasterizerState.DepthClipEnable = TRUE;

    // ブレンドステート (不透明)
    psoDesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
    psoDesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    // 深度ステンシルステート
    psoDesc.DepthStencilState.DepthEnable = TRUE;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    psoDesc.DepthStencilState.StencilEnable = FALSE;

    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = rtvFormat;
    psoDesc.DSVFormat = dsvFormat;
    psoDesc.SampleDesc.Count = 1;

    HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipeline.PSO));

    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create Pipeline State Object (PSO).");
    }

    pipelines[name] = pipeline;
}