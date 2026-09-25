#pragma once

#include "Archive.h"
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
    AssetRef<Texture> texture;

    Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
    Vector2 size = { 100.0f, 100.0f };
    Vector2 pivot = { 0.5f, 0.5f };

    Vector2 uvPos = { 0.0f, 0.0f };
    Vector2 uvScale = { 1.0f, 1.0f };

    bool isVisible = true;
    bool isUI = false;
    SpriteLayer layer = SpriteLayer::Default;

    // テクスチャ設定関数
    void SetTexture(const std::string& filePath)
    {
        texture.Load(filePath);
    }

    template<class Archive>
    void Reflect(Archive& archive) {
        archive.Property("Texture", texture);
        archive.Property("Visible", isVisible);
        archive.Property("IsUI", isUI);
        archive.Property("Color", color);
        archive.Property("Size", size);
        archive.Property("Pivot", pivot);
        archive.Property("UvPos", uvPos);
        archive.Property("UvScale", uvScale);
    }
};