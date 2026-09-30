#include "Engine_pch.h"

#include "Model.h"
#include "Texture.h"
#include "AssetRef.h"

// Modelをロードする関数
void LoadAssetResource(std::shared_ptr<Model>& asset, const std::string& path) {
    if (path.empty()) {
        asset = nullptr;
        return;
    }
    if (!asset) {
        asset = std::make_shared<Model>();
    }
    asset->CreateFromFile(path);
}

// Textureをロードする関数
void LoadAssetResource(std::shared_ptr<Texture>& asset, const std::string& path) {
    if (path.empty()) {
        asset = nullptr;
        return;
    }
    if (!asset) {
        asset = std::make_shared<Texture>();
    }
    int size = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), (int)path.size(), NULL, 0);
    std::wstring wpath(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, path.c_str(), (int)path.size(), &wpath[0], size);
    asset->CreateFromFile(wpath);
}