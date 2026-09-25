// UUID.h
#pragma once
#include <cstdint>
#include <random>

// 被らないランダムな64ビット整数を生成する関数
inline uint64_t GenerateUUID() {
    static std::random_device rd;
    static std::mt19937_64 eng(rd());
    // 0は「無効なID（参照なし）」、1から最大値まで
    static std::uniform_int_distribution<uint64_t> dist(1, 0xFFFFFFFFFFFFFFFF);
    return dist(eng);
}

struct UUIDComponent {
    uint64_t id;

    // コンストラクタで勝手に生成される
    UUIDComponent() : id(GenerateUUID()) {}
    UUIDComponent(uint64_t in_id) : id(in_id) {}

    template<class Archive>
    void Reflect(Archive& archive) {
        // UIやJSONの読み書きに対応させる
        archive.Property("UUID", id);
    }
};