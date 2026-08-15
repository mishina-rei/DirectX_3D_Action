#pragma once
#include "Vector.h"

struct Rigidbody
{
    Vector3 velocity = { 0.0f, 0.0f, 0.0f };     // 速度
    Vector3 acceleration = { 0.0f, 0.0f, 0.0f }; // 加速度
    float mass = 1.0f;                           // 質量
    float drag = 0.0f;                           // 空気抵抗 (0.0 ~ 1.0)
    bool useGravity = true;                      // 重力の影響を受けるか
    bool isKinematic = false;                    // 物理演算の影響を受けないか（trueならスクリプト等で動かす）
};
