#include "Engine_pch.h"
#include "Sprite.h"
#include "GraphicsCore.h"

std::unique_ptr<Mesh> Sprite::m_quadMesh = nullptr;
std::unique_ptr<Material> Sprite::m_material = nullptr;

DirectX::XMFLOAT4X4 Sprite::m_world;
DirectX::XMFLOAT4X4 Sprite::m_view;
DirectX::XMFLOAT4X4 Sprite::m_proj;
DirectX::XMFLOAT2 Sprite::m_offset;
DirectX::XMFLOAT2 Sprite::m_size;
DirectX::XMFLOAT2 Sprite::m_uvPos;
DirectX::XMFLOAT2 Sprite::m_uvScale;
DirectX::XMFLOAT4 Sprite::m_color;
Texture* Sprite::m_texture = nullptr;

void Sprite::Init()
{
    // スプライト用の板ポリゴン（Quad）を作成
    // 中心原点の 1.0 x 1.0 サイズ
    SpriteVertex vertices[] = {
        { {-0.5f,  0.5f, 0.0f}, {0.0f, 0.0f} }, // 左上
        { { 0.5f,  0.5f, 0.0f}, {1.0f, 0.0f} }, // 右上
        { {-0.5f, -0.5f, 0.0f}, {0.0f, 1.0f} }, // 左下
        { { 0.5f, -0.5f, 0.0f}, {1.0f, 1.0f} }  // 右下
    };
    uint32_t indices[] = { 0, 1, 2, 2, 1, 3 };


    auto& gfx = GraphicsCore::Get();
    auto ictx = gfx.GetCommandContext();

    ictx.BeginFrame(gfx.GetCurrentCommandAllocator());

    m_quadMesh = std::make_unique<Mesh>();
    m_quadMesh->Create(vertices, 4, sizeof(SpriteVertex), indices, 6, DXGI_FORMAT_R32_UINT);


    ictx.EndFrame();

    gfx.FlushCommandQueue();

    m_quadMesh->FreeUploadBuffers();

    // スプライト用マテリアルの初期化
    m_material = std::make_unique<Material>("VS_Sprite", "PS_Sprite");

    // スプライトなので透過(アルファブレンド)を有効化、カリングは無効
    m_material->SetTransparent(true);
    m_material->SetCullingDisabled(true);
}

void Sprite::Uninit()
{
    m_quadMesh.reset();
    m_material.reset();
}

void Sprite::Draw()
{
    if (!m_quadMesh || !m_material) return;

    // 定数バッファにデータをセット（HLSL側の変数名に合わせる）
    m_material->SetMatrix("world", DirectX::XMLoadFloat4x4(&m_world));

    DirectX::XMMATRIX viewProj = DirectX::XMLoadFloat4x4(&m_view) * DirectX::XMLoadFloat4x4(&m_proj);
    m_material->SetMatrix("viewProjection", viewProj);

    // offset, size, uvPos, uvScale をまとめてHLSLに送る
    m_material->SetVector("spriteParam", DirectX::XMFLOAT4(m_offset.x, m_offset.y, m_size.x, m_size.y));
    m_material->SetVector("uvParam", DirectX::XMFLOAT4(m_uvPos.x, m_uvPos.y, m_uvScale.x, m_uvScale.y));
    m_material->SetVector("color", m_color);

    // テクスチャのセット
    if (m_texture) {
        m_material->SetTexture(m_texture->GetSrvGpuHandle());
    }

    // マテリアルのバインド (PSO, RootSignature, ConstantBufferのセット)
    m_material->Bind();

    // 描画
    auto& ctx = GraphicsCore::Get().GetCommandContext();
    ctx.GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx.SetVertexBuffer(0, m_quadMesh->GetVertexBufferView());
    ctx.SetIndexBuffer(m_quadMesh->GetIndexBufferView());
    ctx.DrawIndexedInstanced(m_quadMesh->GetIndexCount(), 1, 0, 0, 0);
}

// --- パラメータのセッター群 ---
void Sprite::SetOffset(DirectX::XMFLOAT2 offset) { m_offset = offset; }
void Sprite::SetSize(DirectX::XMFLOAT2 size) { m_size = size; }
void Sprite::SetUVPos(DirectX::XMFLOAT2 pos) { m_uvPos = pos; }
void Sprite::SetUVScale(DirectX::XMFLOAT2 scale) { m_uvScale = scale; }
void Sprite::SetColor(DirectX::XMFLOAT4 color) { m_color = color; }
void Sprite::SetTexture(Texture* tex) { m_texture = tex; }
void Sprite::SetWorld(DirectX::XMFLOAT4X4 world) { m_world = world; }
void Sprite::SetView(DirectX::XMFLOAT4X4 view) { m_view = view; }
void Sprite::SetProjection(DirectX::XMFLOAT4X4 proj) { m_proj = proj; }

void Sprite::SetUIState(bool isUI)
{
}