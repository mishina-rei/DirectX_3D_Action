// Random.h
// 乱数を生成するライブラリのヘルパー

#pragma once
#include <random>

class Random {
public:
    // min以上 max以下の整数を返す
    static int Range(int min, int max);

    // min以上 max未満のfloatを返す
    static float Range(float min, float max);
};