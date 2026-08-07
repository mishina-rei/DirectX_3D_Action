#pragma once

#include "Model.h"
#include "Material.h" // 追加
#include "Vector.h"
#include <memory>

struct MeshRenderer
{
    std::shared_ptr<Model> pModel = nullptr;

    // DX12用マテリアル
    Material material{ "StandardVS", "StandardPS" };

    bool isVisible = true;
    Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };

    MeshRenderer()
    {
        pModel = std::make_shared<Model>();
    }

    MeshRenderer(std::shared_ptr<Model> _pModel, bool visible = true)
        : pModel(_pModel), isVisible(visible)
    {
    }
};