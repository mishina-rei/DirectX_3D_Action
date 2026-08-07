#pragma once
#include "world.h"

class PhysicsSystem
{
public:
    // 物理演算の更新（重力適用、速度による移動など）
    static void Update(ECS::World* world);
};
