#include "DirectX12_pch.h"

#include "ShaderReflection.h"
#include <stdexcept>

ShaderMetadata ShaderReflection::Reflect(ID3DBlob* vsBlob, ID3DBlob* psBlob, ID3D12Device* device)
{
    ShaderMetadata meta;

    std::vector<D3D12_ROOT_PARAMETER> rootParams;
    std::vector<D3D12_DESCRIPTOR_RANGE> srvRanges;

    // ポインタ寿命対策あらかじめ十分なメモリを確保して再配置を防ぐ
    rootParams.reserve(16);
    srvRanges.reserve(16);

    // VSとPSの両方を解析するための配列
    ID3DBlob* blobs[] = { vsBlob, psBlob };

    for (ID3DBlob* blob : blobs) {
        if (!blob) continue;

        ComPtr<ID3D12ShaderReflection> reflector;
        D3DReflect(blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&reflector));

        D3D12_SHADER_DESC shaderDesc;
        reflector->GetDesc(&shaderDesc);

        for (UINT i = 0; i < shaderDesc.BoundResources; ++i) {
            D3D12_SHADER_INPUT_BIND_DESC bindDesc;
            reflector->GetResourceBindingDesc(i, &bindDesc);

            // 重複チェック（VSとPSで同じ b0 や t0 を使っている場合、2重登録を防ぐ）
            bool alreadyExists = false;
            for (const auto& param : rootParams) {
                if (param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_CBV && bindDesc.Type == D3D_SIT_CBUFFER) {
                    if (param.Descriptor.ShaderRegister == bindDesc.BindPoint) alreadyExists = true;
                }
                if (param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE && bindDesc.Type == D3D_SIT_TEXTURE) {
                    // ※厳密には範囲もチェックしますが、簡易的な重複判定
                    alreadyExists = true;
                }
            }
            if (alreadyExists) continue;

            if (bindDesc.Type == D3D_SIT_CBUFFER) {
                // CBVの登録
                D3D12_ROOT_PARAMETER param = {};
                param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
                param.Descriptor.ShaderRegister = bindDesc.BindPoint;
                param.Descriptor.RegisterSpace = bindDesc.Space;
                param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

                meta.cbRootParamIndex = (uint32_t)rootParams.size();
                rootParams.push_back(param);

				// 定数バッファの詳細情報を取得
                ID3D12ShaderReflectionConstantBuffer* cb = reflector->GetConstantBufferByName(bindDesc.Name);
                D3D12_SHADER_BUFFER_DESC cbDesc;

                if (SUCCEEDED(cb->GetDesc(&cbDesc))) {
                    // 1. バッファ全体のサイズ（自動的に16バイト境界にアライメントされたサイズ）を取得
                    meta.cBufferSize = cbDesc.Size;

                    // 2. バッファの中に入っている変数をすべて検出する
                    for (UINT j = 0; j < cbDesc.Variables; ++j) {
                        ID3D12ShaderReflectionVariable* var = cb->GetVariableByIndex(j);
                        D3D12_SHADER_VARIABLE_DESC varDesc;

                        if (SUCCEEDED(var->GetDesc(&varDesc))) {
                            // メモリの開始位置(Offset)、サイズを保存する
                            ShaderVariable shaderVar;
                            shaderVar.offset = varDesc.StartOffset;
                            shaderVar.size = varDesc.Size;

                            // map等で変数名から検索できるように保存する
                            meta.variables[varDesc.Name] = shaderVar;
                        }
                    }
                }
            }
            else if (bindDesc.Type == D3D_SIT_TEXTURE) {
                // テクスチャ(SRV)の登録
                D3D12_DESCRIPTOR_RANGE range = {};
                range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                range.NumDescriptors = 1;
                range.BaseShaderRegister = bindDesc.BindPoint;
                range.RegisterSpace = bindDesc.Space;
                range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

                srvRanges.push_back(range);

                D3D12_ROOT_PARAMETER param = {};
                param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
                param.DescriptorTable.NumDescriptorRanges = 1;
                // 【重要】srvRanges.data() だと常に[0]を指してしまうため、一番最後に追加された要素のアドレスを渡す
                param.DescriptorTable.pDescriptorRanges = &srvRanges.back();
                param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

                meta.texTableRootParamIndex = (uint32_t)rootParams.size();
                rootParams.push_back(param);
            }
        }
    }

    // サンプラー（s0）を静的に追加
    D3D12_STATIC_SAMPLER_DESC sampler = {};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.ShaderRegister = 0;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_ROOT_SIGNATURE_DESC rsDesc = {};
    rsDesc.NumParameters = (UINT)rootParams.size();
    rsDesc.pParameters = rootParams.data();
    rsDesc.NumStaticSamplers = 1;
    rsDesc.pStaticSamplers = &sampler;
    rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> rsBlob, errorBlob;
    D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rsBlob, &errorBlob);
    device->CreateRootSignature(0, rsBlob->GetBufferPointer(), rsBlob->GetBufferSize(), IID_PPV_ARGS(&meta.rootSignature));

    return meta;
}