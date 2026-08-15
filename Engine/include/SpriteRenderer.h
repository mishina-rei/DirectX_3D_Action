#pragma once

#include "Texture.h"
#include "Vector.h"
#include <memory>
#include <string>

enum class SpriteLayer
{
    Default = 0,
    UI_Back,
    UI_Front,
    Overlay,
    Text,
};

struct SpriteRenderer
{
    std::shared_ptr<Texture> pTexture = nullptr;

    Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
    Vector2 size = { 100.0f, 100.0f };
    Vector2 pivot = { 0.5f, 0.5f };

    Vector2 uvPos = { 0.0f, 0.0f };
    Vector2 uvScale = { 1.0f, 1.0f };

    bool isVisible = true;
    bool isUI = false;
    SpriteLayer layer = SpriteLayer::Default;

    // Texture に合わせたロード処理
    void SetTexture(const std::string& filePath)
    {
        pTexture = std::make_shared<Texture>();

        // char文字列からワイド文字(wstring)への変換
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, &filePath[0], (int)filePath.size(), NULL, 0);
        std::wstring wTexPath(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, &filePath[0], (int)filePath.size(), &wTexPath[0], size_needed);

        // テクスチャのロード
        pTexture->CreateFromFile(wTexPath);

        // サイズ設定
        // size = Vector2({float(pTexture->GetWidth()), float(pTexture->GetHeight())});
    }
};