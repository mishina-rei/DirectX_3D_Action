#include "Engine_pch.h"

#include "RenderSystem.h"
#include "MeshRenderer.h"
#include "SpriteRenderer.h"
#include "Transform.h"
#include "CameraSystem.h"
#include "GraphicsCore.h"
#include "Sprite.h"

void RenderSystem::Draw(ECS::World* world)
{
    auto& gfx = GraphicsCore::Get();
    auto* cmdList = gfx.GetCommandList();

    // 描画先のセット (バックバッファと深度バッファ)
    auto rtvHandle = gfx.GetRtvHandle();
    auto dsvHandle = gfx.GetDsvHandle();
    cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    // ディスクリプタヒープのセット (SRV用)
    ID3D12DescriptorHeap* heaps[] = { gfx.GetSrvHeapManager().GetHeap() };
    cmdList->SetDescriptorHeaps(1, heaps);

    // カメラ行列の取得
    DirectX::XMFLOAT4X4 view = CameraSystem::GetView();
    DirectX::XMFLOAT4X4 proj = CameraSystem::GetProjection();
    DirectX::XMMATRIX matView = DirectX::XMLoadFloat4x4(&view);
    DirectX::XMMATRIX matProj = DirectX::XMLoadFloat4x4(&proj);

    // ==================================================
    // 3Dモデル (MeshRenderer) の描画
    // ==================================================
    world->ForEach<MeshRenderer, Transform>([&](ECS::EntityID id, MeshRenderer& mesh, Transform& transform) {
        if (!mesh.isVisible || !mesh.pModel) return;

        // ワールド行列計算
        DirectX::XMMATRIX T = DirectX::XMMatrixTranslationFromVector(transform.position);
        DirectX::XMMATRIX R = DirectX::XMMatrixRotationQuaternion(transform.rotation);
        DirectX::XMMATRIX S = DirectX::XMMatrixScalingFromVector(transform.scale);
        DirectX::XMMATRIX matWorld = S * R * T;

        // マテリアルへ定数バッファのデータを転送
        mesh.material.SetMatrix("world", matWorld);

        mesh.material.SetMatrix("viewProjection", matView * matProj);

        // mesh.material.SetVector("baseColor", DirectX::XMFLOAT4(mesh.color.x, mesh.color.y, mesh.color.z, mesh.color.w));

        // モデル描画
        mesh.pModel->Draw(mesh.material);
        });

    // ==================================================
    // 2Dスプライト (SpriteRenderer) の描画
    // ==================================================
    Sprite::SetView(view);
    Sprite::SetProjection(proj);
    // Sprite::SetUIState(false); // 通常スプライトなのでZテスト有効

    world->ForEach<SpriteRenderer, Transform>([&](ECS::EntityID id, SpriteRenderer& sprite, Transform& transform) {
        if (!sprite.isVisible || !sprite.pTexture || sprite.isUI) return;

        DirectX::XMMATRIX T = DirectX::XMMatrixTranslationFromVector(transform.position);
        DirectX::XMMATRIX R = DirectX::XMMatrixRotationQuaternion(transform.rotation);
        DirectX::XMMATRIX S = DirectX::XMMatrixScalingFromVector(transform.scale);
        DirectX::XMMATRIX matWorld = S * R * T;

        DirectX::XMFLOAT4X4 worldOut;
        DirectX::XMStoreFloat4x4(&worldOut, DirectX::XMMatrixTranspose(matWorld));

        Sprite::SetWorld(worldOut);
        Sprite::SetTexture(sprite.pTexture.get());
        Sprite::SetColor(DirectX::XMFLOAT4(sprite.color.x, sprite.color.y, sprite.color.z, sprite.color.w));
        Sprite::SetSize(DirectX::XMFLOAT2(1.0f, 1.0f));
        Sprite::SetOffset(DirectX::XMFLOAT2((0.5f - sprite.pivot.x) * sprite.size.x, (0.5f - sprite.pivot.y) * sprite.size.y));
        Sprite::SetUVPos(DirectX::XMFLOAT2(sprite.uvPos.x, sprite.uvPos.y));
        Sprite::SetUVScale(DirectX::XMFLOAT2(sprite.uvScale.x, sprite.uvScale.y));

        Sprite::Draw();
        });

    // ==================================================
    // 2Dスプライト (UI) の描画
    // ==================================================
    // UI用の正射影行列
    DirectX::XMFLOAT4X4 uiView;
    DirectX::XMStoreFloat4x4(&uiView, DirectX::XMMatrixTranspose(DirectX::XMMatrixIdentity()));
    DirectX::XMFLOAT4X4 uiProj;
    DirectX::XMStoreFloat4x4(&uiProj, DirectX::XMMatrixTranspose(DirectX::XMMatrixOrthographicOffCenterLH(
        0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f)));

    Sprite::SetView(uiView);
    Sprite::SetProjection(uiProj);
    // Sprite::SetUIState(true); // UIなのでZテスト無効にしたい

    struct UIRenderData {
        SpriteRenderer* sprite;
        Transform* transform;
    };
    std::vector<UIRenderData> uiList;

    world->ForEach<SpriteRenderer, Transform>([&](ECS::EntityID id, SpriteRenderer& sprite, Transform& transform) {
        if (!sprite.isVisible || !sprite.pTexture || !sprite.isUI) return;
        uiList.push_back({ &sprite, &transform });
        });

    // レイヤー順にソート (UI_Back -> UI_Front など)
    std::sort(uiList.begin(), uiList.end(), [](const UIRenderData& a, const UIRenderData& b) {
        return a.sprite->layer < b.sprite->layer;
        });

    for (const auto& data : uiList) {
        SpriteRenderer& sprite = *data.sprite;
        Transform& transform = *data.transform;

        Vector3 scale = transform.scale;
        scale.y *= -1.0f; // 左上原点座標系にするためY反転

        Vector3 pos = transform.position;
        pos.z = 0.0f; // UIのZ座標は0固定

        DirectX::XMMATRIX T = DirectX::XMMatrixTranslationFromVector(pos);
        DirectX::XMMATRIX R = DirectX::XMMatrixRotationQuaternion(transform.rotation);
        DirectX::XMMATRIX S = DirectX::XMMatrixScalingFromVector(scale);
        DirectX::XMMATRIX matWorld = S * R * T;

        DirectX::XMFLOAT4X4 worldOut;
        DirectX::XMStoreFloat4x4(&worldOut, DirectX::XMMatrixTranspose(matWorld));

        Sprite::SetWorld(worldOut);
        Sprite::SetTexture(sprite.pTexture.get());
        Sprite::SetColor(DirectX::XMFLOAT4(sprite.color.x, sprite.color.y, sprite.color.z, sprite.color.w));
        Sprite::SetSize(DirectX::XMFLOAT2(sprite.size.x * sprite.uvScale.x, sprite.size.y * sprite.uvScale.y));
        Sprite::SetOffset(DirectX::XMFLOAT2((0.5f - sprite.pivot.x) * sprite.size.x, (0.5f - sprite.pivot.y) * sprite.size.y));
        Sprite::SetUVPos(DirectX::XMFLOAT2(sprite.uvPos.x, sprite.uvPos.y));
        Sprite::SetUVScale(DirectX::XMFLOAT2(sprite.uvScale.x, sprite.uvScale.y));

        Sprite::Draw();
    }
}