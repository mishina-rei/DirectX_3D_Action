#include "SceneTransitionTestScript.h"
#include "Archive.h"
#include "EngineTime.h"
#include "SceneManager.h"

void SceneTransitionTestScript::OnCreate()
{
    elapsedTime = 0.0f;
    hasRequestedSceneLoad = false;
}

void SceneTransitionTestScript::Update()
{
    if (hasRequestedSceneLoad || targetScene.empty())
    {
        return;
    }

    elapsedTime += Time::GetDeltaTime();
    if (elapsedTime >= delaySeconds)
    {
        hasRequestedSceneLoad =
            SceneManager::GetInstance().RequestSceneLoad(targetScene);
    }
}

void SceneTransitionTestScript::SaveSettings(JsonWriteArchive& archive)
{
    archive.Property("TargetScene", targetScene);
    archive.Property("DelaySeconds", delaySeconds);
}

void SceneTransitionTestScript::LoadSettings(JsonReadArchive& archive)
{
    archive.Property("TargetScene", targetScene);
    archive.Property("DelaySeconds", delaySeconds);
    if (delaySeconds < 0.0f)
    {
        delaySeconds = 0.0f;
    }
}

#if WITH_EDITOR
// エディタ上でのみ設定UIを描画する
void SceneTransitionTestScript::DrawSettings(ImGuiArchive& archive)
{
    archive.Property("TargetScene", targetScene);
    archive.Property("DelaySeconds", delaySeconds);
    if (delaySeconds < 0.0f)
    {
        delaySeconds = 0.0f;
    }
}
#endif
