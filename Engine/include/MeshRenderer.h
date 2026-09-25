#pragma once

#include "Archive.h"
#include "Model.h"
#include "Material.h"
#include "Vector.h"
#include <memory>

struct MeshRenderer
{
    AssetRef<Model> model;

    // DX12用マテリアル
    std::shared_ptr<Material> material = nullptr;

    bool isVisible = true;
    Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };

    MeshRenderer()
    {
        material = std::make_shared<Material>("VS_Standard", "PS_Standard");
    }

    MeshRenderer(std::shared_ptr<Model> _pModel, bool visible = true)
        : isVisible(visible)
    {
        model.asset = _pModel;
        material = std::make_shared<Material>("VS_Standard", "PS_Standard");
    }

    void SetModel(const std::string& filePath)
    {
        model.Load(filePath);
    }

    template<class Archive>
    void Reflect(Archive& archive) {
        archive.Property("Model", model);
        archive.Property("Visible", isVisible);
        archive.Property("Color", color);
    }
};