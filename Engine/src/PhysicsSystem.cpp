#include "Engine_pch.h"
#include "PhysicsSystem.h"
#include "Rigidbody.h"
#include "Transform.h"
#include <cmath>

namespace
{
    // 1単位を1mとした重力加速度
    constexpr float GRAVITY_ACCELERATION = -9.8f;
}

void PhysicsSystem::Update(ECS::World* world, float deltaTime)
{
    if (deltaTime <= 0.0f)
    {
        return;
    }

    world->ForEach<Rigidbody, Transform>(
        [&](ECS::EntityID id, Rigidbody& rb, Transform& trans)
        {
            if (rb.isKinematic)
            {
                return;
            }

			Vector3 acceleration = rb.acceleration; // 外部からの加速度を取得

            if (rb.useGravity)
            {
				acceleration.y += GRAVITY_ACCELERATION; // 重力加速度を加算
            }

			rb.velocity += acceleration * deltaTime;    // 速度を更新

            if (rb.drag > 0.0f)
            {
				rb.velocity *= std::exp(-rb.drag * deltaTime);  // ドラッグによる減速を適用
            }

			trans.position += rb.velocity * deltaTime;  // 位置を更新
        });
}
