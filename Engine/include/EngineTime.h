#pragma once

#include <chrono>

class EngineCore;

class Time
{
private:
    friend class EngineCore;

    static void Reset();
    static void Update();

    static constexpr float MAX_DELTA_TIME = 0.1f;

    static std::chrono::steady_clock::time_point previousTime;

    static float deltaTime;             // 前回の更新からの経過秒数（最大0.1秒）
    static float timeScale;             // 時間スケール

public:
    
    static float GetDeltaTime() { return deltaTime * timeScale; };            // 前回の更新からの経過秒数（最大0.1秒）
    static float GetUnscaledDeltaTime() { return deltaTime; };    // 前回の更新からの経過秒数（時間スケールを考慮しない）
	static float GetTimeScale() { return timeScale; };            // 時間スケール
	static void SetTimeScale(float scale) { timeScale = scale; };  // 時間スケールを設定


};
