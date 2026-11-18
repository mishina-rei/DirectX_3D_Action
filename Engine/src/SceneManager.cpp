#include "Engine_pch.h"

#include "SceneManager.h"
#include "EffekseerManager.h"
#include "GraphicsCore.h"
#include "SceneSerializer.h"
#include "Script.h"
#include <utility>

SceneManager::SceneManager()
{
}

void SceneManager::Update()
{
    ChangeScene();
    if (!ProcessSceneLoad())
    {
        return;
    }
    currentScene->Update();
	currentScene->ExecuteCommands(); // 遅延させていたWorldへの変更を適用
}

void SceneManager::Draw()
{
	currentScene->Draw();
}

void SceneManager::Uninit()
{
    canLoadScene = false;
    ResetSceneLoadState();
    if (currentScene)
    {
        currentScene->Uninit();
        delete currentScene;
        currentScene = nullptr;
    }
}

void SceneManager::ChangeScene()
{
    if (!canLoadScene || nextScene == nullptr)
    {
        return;
    }

    isChangingScene = true;
    GraphicsCore::Get().FlushCommandQueue();
    EffekseerManager::StopAll();
    if (currentScene)
	{
		currentScene->Uninit();
		delete currentScene;
	}
	EffekseerManager::ClearCache();
	currentScene = nextScene;
	nextScene = nullptr;
    currentScene->Init();
    currentScene->ExecuteCommands(); // 初期化時に溜まったコマンドを実行

    // 初期化する
    loadedScenePath.clear();    
    sceneLoadError.clear();
    ++sceneRevision;
    isChangingScene = false;
}

bool SceneManager::RequestSceneLoad(const std::string& filepath)
{
    if (!canLoadScene || isChangingScene || !currentScene ||
        filepath.empty() || nextScene || !pendingScenePath.empty())
    {
        return false;   // パスの状態とシーンマネージャーの状態をチェックする
    }

    pendingScenePath = filepath;
    return true;
}

void SceneManager::ResetSceneLoadState()
{
    pendingScenePath.clear();
    loadedScenePath.clear();
    sceneLoadError.clear();
    delete nextScene;
    nextScene = nullptr;
}

bool SceneManager::ProcessSceneLoad()
{
    if (!canLoadScene || pendingScenePath.empty())
    {
        return true;
    }

    const std::string filepath = std::move(pendingScenePath);
    pendingScenePath.clear();
    isChangingScene = true;

    // 読み込み失敗時も古いInspector参照を残さないよう変更を通知する
    ++sceneRevision;
    EffekseerManager::StopAll();

    // ロードする
    const bool isLoaded = SceneSerializer::Load(currentScene->GetWorld(), filepath,currentScene->GetCommandBuffer(), sceneLoadError);

    isChangingScene = false;
    if (!isLoaded)
    {
        return false;   // ロードできてなかったらエラー
    }

    loadedScenePath = filepath;

    // スクリプトが正しくロードされたかチェック
    bool hasUnavailableScript = false;
    currentScene->GetWorld()->ForEachComponent<Script>(
        [&](ECS::EntityID id, Script& script) {
            if (script.HasUnavailableType())
            {
                hasUnavailableScript = true;
            }
        });

    if (hasUnavailableScript)   // スクリプトが正しくロードされなかったらエラー
    {
        sceneLoadError = "Cannot play: a script in the loaded scene is unavailable.";
        return false;
    }

    // Scriptの初期化は、この後のScene::Update()で行う
    return true;
}
