#pragma once
#include "Vector.h"

struct Rigidbody
{
    Vector3 velocity = { 0.0f, 0.0f, 0.0f };     // 速度（ワールド単位/秒）
    Vector3 acceleration = { 0.0f, 0.0f, 0.0f }; // 加速度（ワールド単位/秒²）
    float mass = 1.0f;                           // 質量
    float drag = 0.0f;                           // 減衰係数（1/秒、0以上）
    bool useGravity = true;                      // 重力の影響を受けるか
    bool isKinematic = false;                    // 物理演算の影響を受けないか（trueならスクリプト等で動かす）

    template<class Archive>
    void Reflect(Archive& archive) {
        archive.Property("Mass", mass);
        archive.Property("Drag", drag);
        archive.Property("UseGravity", useGravity);
        archive.Property("IsKinematic", isKinematic);
        archive.Property("Velocity", velocity);
        archive.Property("Acceleration", acceleration);
    }
};
