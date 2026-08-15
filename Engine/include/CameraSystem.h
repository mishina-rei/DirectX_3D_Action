#pragma once
#include "world.h"
#include <DirectXMath.h>

class CameraSystem
{
public:
    
    static DirectX::XMFLOAT4X4 GetView();       // ビュー行列
    static DirectX::XMFLOAT4X4 GetProjection(); // プロジェクション行列

    static void SetCamera(ECS::EntityID id) { cameraId = id; }
    static ECS::EntityID GetCamera() { return cameraId; }

private:

    // メインカメラ(カメラは1個とする)
    static ECS::EntityID cameraId;
};
