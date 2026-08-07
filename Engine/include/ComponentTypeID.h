// ComponentTypeID.h

#pragma once

#include <cstdint>

namespace ECS
{
    // 高速な型ID生成システム
    // std::type_index の代わりに int を使う
    using ComponentTypeID = uint32_t;

    class ComponentRegistry
    {
        static inline ComponentTypeID nextId = 0;

    public:
        template <typename T>
        static ComponentTypeID GetID() {
            // 関数内のstatic変数は、テンプレートの型ごとに1回だけ初期化される
            static ComponentTypeID id = nextId++;
            return id;
        }
    };
}