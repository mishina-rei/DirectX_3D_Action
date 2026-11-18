#pragma once

#include "NativeScript.h"
#include <string>

// 指定した秒数が経過したらシーン遷移を1回予約する確認用スクリプト
class SceneTransitionTestScript : public NativeScript
{
public:
    void OnCreate() override;
    void Update() override;

    void SaveSettings(JsonWriteArchive& archive) override;
    void LoadSettings(JsonReadArchive& archive) override;
#if WITH_EDITOR
    void DrawSettings(ImGuiArchive& archive) override;
#endif

private:
    std::string targetScene;
    float delaySeconds = 3.0f;
    float elapsedTime = 0.0f;
    bool hasRequestedSceneLoad = false;
};
