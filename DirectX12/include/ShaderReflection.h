#pragma once
#include <d3d12.h>
#include <dxcapi.h>
#include <wrl/client.h>
#include <string>
#include <unordered_map>
#include <vector>

using Microsoft::WRL::ComPtr;

// 定数バッファ内の個々の変数（例: "u_Color"）の情報
struct ShaderVariable {
    uint32_t offset;
    uint32_t size;
};

// シェーダーが要求するリソース全体のメタデータ
struct ShaderMetadata {
    ComPtr<ID3D12RootSignature> rootSignature;

    // 変数名 -> その変数がバッファの何バイト目にあるか
    std::unordered_map<std::string, ShaderVariable> variables;

    // 定数バッファ全体のサイズ（256バイトアライメント前）
    uint32_t cBufferSize = 0;

    // Root Signatureにおける、定数バッファ(b0)のインデックス
    uint32_t cbRootParamIndex = 0;

    // Root Signatureにおける、テクスチャテーブル(t0)のインデックス
    uint32_t texTableRootParamIndex = 1;
};

class ShaderReflection {
public:
    static ShaderMetadata Reflect(IDxcBlob* vsBlob, IDxcBlob* psBlob, ID3D12Device* device);
};