#include "Engine_pch.h"
#include "EngineTime.h"

std::chrono::steady_clock::time_point Time::previousTime{};
float Time::deltaTime = 0.0f;
float Time::timeScale = 1.0f;

void Time::Reset()
{
    previousTime = std::chrono::steady_clock::now();
    deltaTime = 0.0f;
}

void Time::Update()
{
    const auto currentTime = std::chrono::steady_clock::now();

    const float elapsedSeconds = std::chrono::duration<float>(currentTime - previousTime).count();

    previousTime = currentTime;

	// 経過時間が最大値を超えた場合は最大値に制限
    deltaTime = elapsedSeconds > MAX_DELTA_TIME ? MAX_DELTA_TIME : elapsedSeconds;
}
