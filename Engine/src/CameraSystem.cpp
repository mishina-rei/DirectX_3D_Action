#include "Engine_pch.h"

#include "CameraSystem.h"
#include "SceneManager.h"
#include "Camera.h"
#include "Transform.h"

ECS::EntityID CameraSystem::cameraId = 0;

DirectX::XMFLOAT4X4 CameraSystem::GetView()
{
    ECS::World* world = SceneManager::GetInstance().GetCurrentScene()->GetWorld();

    // カメラコンポーネントを持っていたら計算を行う
    if(world->HasComponent<Camera>(cameraId))
    {
        Vector3 pos = { 0.0f, 0.0f, 0.0f };
        Quaternion rot = Quaternion::Identity();

        // トランスフォームコンポーネントを持っていたらそのデータを使う
        if(world->HasComponent<Transform>(cameraId))
        {
            Transform& transform = world->GetComponent<Transform>(cameraId);
            pos = transform.position;
            rot = transform.rotation;
        }
        return world->GetComponent<Camera>(cameraId).GetViewMatrix(pos, rot);
    }

    // カメラコンポーネントを持っていないなら単位行列を返す
    DirectX::XMFLOAT4X4 mat;
    DirectX::XMStoreFloat4x4(&mat, DirectX::XMMatrixIdentity());
    return mat;
}

DirectX::XMFLOAT4X4 CameraSystem::GetProjection()
{
    ECS::World* world = SceneManager::GetInstance().GetCurrentScene()->GetWorld();

    // カメラコンポーネントを持っていたらそのプロジェクション行列を返す
    if(world->HasComponent<Camera>(cameraId))
    {
        return world->GetComponent<Camera>(cameraId).GetProjectionMatrix();
    }
    DirectX::XMFLOAT4X4 mat;
    DirectX::XMStoreFloat4x4(&mat, DirectX::XMMatrixIdentity());
    return mat;
}
