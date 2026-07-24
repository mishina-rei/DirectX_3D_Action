#pragma once
#include <d3d12.h>
#include <dxcapi.h> // FXC(d3dcompiler.h)からDXCへ変更
#include <wrl/client.h>
#include <string>
#include <unordered_map>
#include <vector>

using Microsoft::WRL::ComPtr;

class ShaderManager {
public:
    static ShaderManager& Get() {
        static ShaderManager instance;
        return instance;
    }

    // DXCの初期化処理を追加するため、初期化で呼び出します
    void Initialize(ID3D12Device* _device);

    // DXCは引数にワイド文字列(L"...")を使うため、wchar_tに変更
    void LoadShader(const std::string& name, const std::wstring& filePath, const wchar_t* entryPoint, const wchar_t* targetProfile);
    void LoadVS(const std::string& name, const std::wstring& filePath);
    void LoadPS(const std::string& name, const std::wstring& filePath);

    // ID3DBlobではなくIDxcBlobを返す
    IDxcBlob* GetShader(const std::string& name) {
        auto it = shaders.find(name);
        if (it != shaders.end()) {
            return it->second.Get();
        }
        return nullptr;
    }

	IDxcUtils* GetDxcUtils() {
		return dxcUtils.Get();
	}

private:
    ShaderManager() = default;

    ID3D12Device* device = nullptr;

    // DXCのコンパイラとユーティリティ
    ComPtr<IDxcUtils> dxcUtils;
    ComPtr<IDxcCompiler3> dxcCompiler;

    std::unordered_map<std::string, ComPtr<IDxcBlob>> shaders;

    ComPtr<IDxcBlob> CompileShader(const std::wstring& filePath, const wchar_t* entryPoint, const wchar_t* targetProfile);
};