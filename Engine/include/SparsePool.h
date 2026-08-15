// SparsePool.h

#pragma once

#include "EntityID.h"

namespace ECS {

    // スパースセット用のストレージ
    class ISparsePool {
    public:
        virtual ~ISparsePool() = default;
        virtual void Remove(EntityID id) = 0;
    };

    template <typename T>
    class SparsePool : public ISparsePool {
    public:
        std::unordered_map<EntityID, T> data;

        void Set(EntityID id, T val) {
            data[id] = val;
        }

        T& Get(EntityID id) {
            return data[id];
        }

        void Remove(EntityID id) override {
            data.erase(id);
        }
    };
}
