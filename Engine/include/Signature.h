// Signature.h

#pragma once
#include <vector>
#include <algorithm>
#include <set>

#include "ComponentTypeID.h"

namespace ECS
{
    // アーキタイプを識別するための署名
    // std::set ではなく sorted vector を使うことでキャッシュ効率向上
    using Signature = std::vector<ComponentTypeID>;

    inline void AddToSignature(Signature& sig, ComponentTypeID id) {
        sig.push_back(id);
        std::sort(sig.begin(), sig.end()); // 常にソートして一意性を保つ
    }

    inline void RemoveFromSignature(Signature& sig, ComponentTypeID id) {
        auto it = std::remove(sig.begin(), sig.end(), id);
        sig.erase(it, sig.end());
    }
}