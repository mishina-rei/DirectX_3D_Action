#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <unordered_map>

// PSO生成に必要なキー情報
struct PSOKey {
    ID3D12RootSignature* rootSig;
    IDxcBlob* VS;
    IDxcBlob* PS;
    bool isTransparent;
    bool disableCulling;
    DXGI_FORMAT rtvFormat;
    DXGI_FORMAT dsvFormat;

    bool operator==(const PSOKey& o) const {
        return rootSig == o.rootSig && VS == o.VS && PS == o.PS &&
            isTransparent == o.isTransparent && disableCulling == o.disableCulling &&
            rtvFormat == o.rtvFormat && dsvFormat == o.dsvFormat;
    }
};

// 単純なハッシュ関数
struct PSOKeyHasher {
    size_t operator()(const PSOKey& k) const {
        size_t h = 0;
        h ^= std ::hash<void*>{}(k.rootSig) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<void*>{}(k.VS) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<void*>{}(k.PS) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<bool>{}(k.isTransparent) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};

class PSOCache {
public:
    static PSOCache& Get() { static PSOCache instance; return instance; }

    ID3D12PipelineState* GetOrCreatePSO(ID3D12Device* device, const PSOKey& key);

private:
    std::unordered_map<PSOKey, Microsoft::WRL::ComPtr<ID3D12PipelineState>, PSOKeyHasher> cache;
};