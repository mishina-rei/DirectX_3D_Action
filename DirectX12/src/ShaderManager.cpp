#include "DirectX12_pch.h"
#include "ShaderManager.h"
#include <stdexcept>

// DXCのライブラリをリンク
#pragma comment(lib, "dxcompiler.lib")

void ShaderManager::Initialize(ID3D12Device* _device) {
    device = _device;

    // DXCコンパイラのインスタンスを生成
    HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
    if (FAILED(hr)) throw std::runtime_error("Failed to create DxcUtils.");

    hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
    if (FAILED(hr)) throw std::runtime_error("Failed to create DxcCompiler.");
}

ComPtr<IDxcBlob> ShaderManager::CompileShader(const std::wstring& filePath, const wchar_t* entryPoint, const wchar_t* targetProfile) {
    // ファイルの読み込み
    ComPtr<IDxcBlobEncoding> sourceBlob;
    HRESULT hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, &sourceBlob);
    if (FAILED(hr)) {
        std::string fileStr(filePath.begin(), filePath.end());
        throw std::runtime_error("Failed to load shader file: " + fileStr);
    }

    // コンパイル引数の設定
    std::vector<LPCWSTR> args = {
        filePath.c_str(),            // エラー出力に表示されるファイル名
        L"-E", entryPoint,           // エントリーポイント
        L"-T", targetProfile,        // ターゲットプロファイル (vs_6_0 など)
        L"-Zpr",                     // 行列をRow-Majorで扱う (C++のDirectXMathと合わせるのに便利)
        L"-HV", L"2021"              // HLSL 2021標準を使用 (最新機能を使うため)
    };

#if defined(_DEBUG)
    args.push_back(L"-Zi");           // デバッグ情報を含める
    args.push_back(L"-Qembed_debug"); // デバッグ情報をバイナリに埋め込む
    args.push_back(L"-Od");           // 最適化を無効化
#endif

    // インクルードハンドラの作成 (#includeを解決するため)
    ComPtr<IDxcIncludeHandler> includeHandler;
    dxcUtils->CreateDefaultIncludeHandler(&includeHandler);

    // ソースコードバッファの定義
    DxcBuffer sourceBuffer;
    sourceBuffer.Ptr = sourceBlob->GetBufferPointer();
    sourceBuffer.Size = sourceBlob->GetBufferSize();
    sourceBuffer.Encoding = DXC_CP_UTF8;

    // コンパイルの実行
    ComPtr<IDxcResult> result;
    hr = dxcCompiler->Compile(
        &sourceBuffer,
        args.data(),
        (UINT32)args.size(),
        includeHandler.Get(),
        IID_PPV_ARGS(&result)
    );

    // エラーチェック
    ComPtr<IDxcBlobUtf8> errors;
    result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
    if (errors != nullptr && errors->GetStringLength() > 0) {
        OutputDebugStringA(errors->GetStringPointer());

        // コンパイルのステータスを確認
        HRESULT status;
        result->GetStatus(&status);
        if (FAILED(status)) {
            throw std::runtime_error("Shader Compile Error:\n" + std::string(errors->GetStringPointer()));
        }
    }

    // コンパイル済みバイナリ(Blob)の取得
    ComPtr<IDxcBlob> shaderObj;
    result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderObj), nullptr);

    return shaderObj;
}

void ShaderManager::LoadShader(const std::string& name, const std::wstring& filePath, const wchar_t* entryPoint, const wchar_t* targetProfile) {
    if (shaders.count(name)) return;
    shaders[name] = CompileShader(filePath, entryPoint, targetProfile);
}

void ShaderManager::LoadVS(const std::string& name, const std::wstring& filePath) {
    // Shader Model 6.0 を指定
    LoadShader(name, filePath, L"main", L"vs_6_0");
}

void ShaderManager::LoadPS(const std::string& name, const std::wstring& filePath) {
    // Shader Model 6.0 を指定
    LoadShader(name, filePath, L"main", L"ps_6_0");
}