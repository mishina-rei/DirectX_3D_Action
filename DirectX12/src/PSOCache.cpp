#include "DirectX12_pch.h"

#include <stdexcept>
#include "PSOCache.h"
#include "ShaderManager.h"

using Microsoft::WRL::ComPtr;


// 解析時にポインタが消えないよう、SemanticNameの文字列を保持する構造体
struct DynamicInputLayout {
    std::vector<std::string> SemanticNames;
    std::vector<D3D12_INPUT_ELEMENT_DESC> Elements;
};

// プロトタイプ宣言
DynamicInputLayout GenerateInputLayoutFromVS(IDxcBlob* vsBlob);

ID3D12PipelineState* PSOCache::GetOrCreatePSO(ID3D12Device* device, const PSOKey& key) {
    auto it = cache.find(key);
    if (it != cache.end()) {
        return it->second.Get();
    }

    // PSOの生成ロジック
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    ZeroMemory(&psoDesc, sizeof(psoDesc));

    // --- シェーダーとルートシグネチャ ---
    psoDesc.pRootSignature = key.rootSig;
    psoDesc.VS = { reinterpret_cast<UINT8*>(key.VS->GetBufferPointer()), key.VS->GetBufferSize() };
    psoDesc.PS = { reinterpret_cast<UINT8*>(key.PS->GetBufferPointer()), key.PS->GetBufferSize() };

    // --- 頂点レイアウト (エンジン標準の3Dモデル用と仮定) ---
    auto dynamicLayout = GenerateInputLayoutFromVS(key.VS);

    psoDesc.InputLayout = { dynamicLayout.Elements.data(), (UINT)(dynamicLayout.Elements.size()) };

    // --- ラスタライザーステート (カリング設定の反映) ---
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
    psoDesc.RasterizerState.DepthClipEnable = TRUE;

    if (key.disableCulling) {
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // 両面描画（草や葉っぱなど）
    }
    else {
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK; // 通常の背面カリング
    }

    // --- ブレンドステート (透明設定の反映) ---
    psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
    psoDesc.BlendState.IndependentBlendEnable = FALSE;

    if (key.isTransparent) {
        // 半透明ブレンド (Alpha Blend)
        psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
        psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    }
    else {
        // 不透明 (上書き)
        psoDesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
    }
    psoDesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    // --- 深度ステンシルステート ---
    psoDesc.DepthStencilState.DepthEnable = TRUE;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    psoDesc.DepthStencilState.StencilEnable = FALSE;

    if (key.isTransparent) {
        // 半透明オブジェクトは深度テストはするが、深度バッファへの書き込みは行わない
        psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    }
    else {
        psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    }

    // --- その他の固定設定 ---
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = key.rtvFormat;
    psoDesc.DSVFormat = key.dsvFormat;
    psoDesc.SampleDesc.Count = 1;

    // 実際のAPI呼び出し
    Microsoft::WRL::ComPtr<ID3D12PipelineState> newPSO;
    HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&newPSO));
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create Pipeline State Object.");
    }

    cache[key] = newPSO;

    return newPSO.Get();
}

// リフレクションから型（DXGI_FORMAT）を判定するヘルパー
DXGI_FORMAT DetermineFormat(BYTE mask, D3D_REGISTER_COMPONENT_TYPE componentType) {
    if (mask == 1) { // 1成分 (x)
        if (componentType == D3D_REGISTER_COMPONENT_UINT32) return DXGI_FORMAT_R32_UINT;
        if (componentType == D3D_REGISTER_COMPONENT_SINT32) return DXGI_FORMAT_R32_SINT;
        if (componentType == D3D_REGISTER_COMPONENT_FLOAT32) return DXGI_FORMAT_R32_FLOAT;
    }
    else if (mask <= 3) { // 2成分 (xy)
        if (componentType == D3D_REGISTER_COMPONENT_UINT32) return DXGI_FORMAT_R32G32_UINT;
        if (componentType == D3D_REGISTER_COMPONENT_SINT32) return DXGI_FORMAT_R32G32_SINT;
        if (componentType == D3D_REGISTER_COMPONENT_FLOAT32) return DXGI_FORMAT_R32G32_FLOAT;
    }
    else if (mask <= 7) { // 3成分 (xyz)
        if (componentType == D3D_REGISTER_COMPONENT_UINT32) return DXGI_FORMAT_R32G32B32_UINT;
        if (componentType == D3D_REGISTER_COMPONENT_SINT32) return DXGI_FORMAT_R32G32B32_SINT;
        if (componentType == D3D_REGISTER_COMPONENT_FLOAT32) return DXGI_FORMAT_R32G32B32_FLOAT;
    }
    else if (mask <= 15) { // 4成分 (xyzw)
        if (componentType == D3D_REGISTER_COMPONENT_UINT32) return DXGI_FORMAT_R32G32B32A32_UINT;
        if (componentType == D3D_REGISTER_COMPONENT_SINT32) return DXGI_FORMAT_R32G32B32A32_SINT;
        if (componentType == D3D_REGISTER_COMPONENT_FLOAT32) return DXGI_FORMAT_R32G32B32A32_FLOAT;
    }
    return DXGI_FORMAT_UNKNOWN;
}

// 頂点シェーダーのBlobからInputLayoutを自動生成する関数
DynamicInputLayout GenerateInputLayoutFromVS(IDxcBlob* vsBlob) {
    DynamicInputLayout layout;
	IDxcUtils* dxcUtils = ShaderManager::Get().GetDxcUtils();
    // 1. DXC用のバッファ構造体を準備
    DxcBuffer reflectionData;
    reflectionData.Ptr = vsBlob->GetBufferPointer();
    reflectionData.Size = vsBlob->GetBufferSize();
    reflectionData.Encoding = DXC_CP_ACP;

    // 2. DXCのインターフェース経由でリフレクションを取得
    Microsoft::WRL::ComPtr<ID3D12ShaderReflection> reflector;
    HRESULT hr = dxcUtils->CreateReflection(&reflectionData, IID_PPV_ARGS(&reflector));
    if (FAILED(hr)) {
        // 必要に応じてエラーハンドリング
        throw std::runtime_error("Failed to create shader reflection.");
    }

    D3D12_SHADER_DESC shaderDesc;
    reflector->GetDesc(&shaderDesc);

    layout.SemanticNames.reserve(shaderDesc.InputParameters);
    layout.Elements.reserve(shaderDesc.InputParameters);

    for (UINT i = 0; i < shaderDesc.InputParameters; ++i) {
        D3D12_SIGNATURE_PARAMETER_DESC paramDesc;
        reflector->GetInputParameterDesc(i, &paramDesc);

        // 文字列ポインタの寿命切れを防ぐため、std::stringにコピーして保持
        layout.SemanticNames.push_back(paramDesc.SemanticName);

        D3D12_INPUT_ELEMENT_DESC element = {};
        element.SemanticIndex = paramDesc.SemanticIndex;
        element.InputSlot = 0; // 基本は0スロット

        // 自動で直前の要素の末尾にオフセットを繋げてくれる
        element.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

        element.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        element.InstanceDataStepRate = 0;

        // マスク（使われている成分数）と型から DXGI_FORMAT を逆算
        element.Format = DetermineFormat(paramDesc.Mask, paramDesc.ComponentType);

        layout.Elements.push_back(element);
    }

    // 全要素の push_back が終わって「絶対にメモリアドレスが変わらない状態」
    // になってから、まとめてポインタを繋ぐ
    for (size_t i = 0; i < layout.Elements.size(); ++i) {
        layout.Elements[i].SemanticName = layout.SemanticNames[i].c_str();
    }

    return layout;
}