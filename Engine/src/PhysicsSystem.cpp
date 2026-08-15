#include "Engine_pch.h"
#include "PhysicsSystem.h"
#include "Rigidbody.h"
#include "Transform.h"
#include "Defines.h"

void PhysicsSystem::Update(ECS::World* world)
{
    Vector3 gravity = { 0.0f, -0.016f, 0.0f }; // 重力加速度

    world->ForEach<Rigidbody, Transform>([&](ECS::EntityID id, Rigidbody& rb, Transform& trans) {
        if (rb.isKinematic) return;

        // 重力の適用
        if (rb.useGravity) {
            rb.velocity += gravity;
        }

        // 加速度の適用
        rb.velocity += rb.acceleration;

        // 空気抵抗の適用
        if (rb.drag > 0.0f) {
            rb.velocity *= (1.0f - rb.drag);
        }

        // 速度による位置更新
        trans.position += rb.velocity;
    });
}
