#pragma once

// アセット読み込み用のヘルパー関数
class Model;
class Texture;

void LoadAssetResource(std::shared_ptr<Model>& asset, const std::string& path);
void LoadAssetResource(std::shared_ptr<Texture>& asset, const std::string& path);

// アセットのファイルパスと読み込み済み実体を管理する型
template<typename T>
struct AssetRef {
    std::string path = "";
    std::shared_ptr<T> asset = nullptr;

    void Load(const std::string& newPath) {
        path = newPath;
        LoadAssetResource(asset, path);
    }

    T* operator->() const { return asset.get(); }
    T* get() const { return asset.get(); }
    operator bool() const { return asset != nullptr; }
};
