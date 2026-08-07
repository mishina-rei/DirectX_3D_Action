#pragma once

#include <DirectXMath.h>
#include <memory>
#include "Mesh.h"
#include "Material.h"
#include "Texture.h"

struct SpriteVertex
{
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT2 uv;
};

class Sprite
{
public:
    static void Init();
    static void Uninit();
    static void Draw();

    // パラメータ設定用
    static void SetOffset(DirectX::XMFLOAT2 offset);
    static void SetSize(DirectX::XMFLOAT2 size);
    static void SetUVPos(DirectX::XMFLOAT2 pos);
    static void SetUVScale(DirectX::XMFLOAT2 scale);
    static void SetColor(DirectX::XMFLOAT4 color);
    static void SetTexture(Texture* tex);

    static void SetWorld(DirectX::XMFLOAT4X4 world);
    static void SetView(DirectX::XMFLOAT4X4 view);
    static void SetProjection(DirectX::XMFLOAT4X4 proj);

    // UI描画用にZテストやカリングを無効にする設定
    static void SetUIState(bool isUI);

private:
    static std::unique_ptr<Mesh> m_quadMesh;
    static std::unique_ptr<Material> m_material; // 先ほど作ったMaterialクラス！

    // CPU側で一時保持するパラメータ
    static DirectX::XMFLOAT4X4 m_world;
    static DirectX::XMFLOAT4X4 m_view;
    static DirectX::XMFLOAT4X4 m_proj;
    static DirectX::XMFLOAT2 m_offset;
    static DirectX::XMFLOAT2 m_size;
    static DirectX::XMFLOAT2 m_uvPos;
    static DirectX::XMFLOAT2 m_uvScale;
    static DirectX::XMFLOAT4 m_color;
    static Texture* m_texture;
};