// SceneManager.h

#pragma once

#include "Scene.h"
#include <cstdint>
#include <string>

class SceneManager
{
public:

	static SceneManager& GetInstance()
	{
		static SceneManager instance;
		return instance;
	}

	SceneManager(const SceneManager&) = delete;
	SceneManager& operator=(const SceneManager&) = delete;

	Scene* GetCurrentScene() { return currentScene; }

	template <class T>
	void Init();
	void Update();
	void Draw();
	void Uninit();

	// 次のシーンを設定
	template <class T>
    void SetScene()
    {
        if (!canLoadScene || isChangingScene)
        {
            return;
        }
        delete nextScene;
        nextScene = new T();
        pendingScenePath.clear();
    }

    // 更新中は予約だけ行い、次フレームの更新開始時に実行する
    bool RequestSceneLoad(const std::string& filepath);
    void SetSceneLoadEnabled(bool isEnabled) { canLoadScene = isEnabled; }
    void ResetSceneLoadState();

    uint64_t GetSceneRevision() const { return sceneRevision; }
    const std::string& GetSceneLoadError() const { return sceneLoadError; }
    const std::string& GetLoadedScenePath() const { return loadedScenePath; }

private:

	SceneManager();

	// シーン変える
	void ChangeScene();


    Scene* currentScene = nullptr;
    Scene* nextScene = nullptr;

    std::string pendingScenePath;   // 次のシーンのパス
    std::string loadedScenePath;    // ロードしたシーンのパス
    std::string sceneLoadError;     // ロード時に出たエラーを保持する
    uint64_t sceneRevision = 0;
    bool canLoadScene = true;       // シーンをロードできる状態かどうか
    bool isChangingScene = false;   // シーンを変更できるかどうか

    bool ProcessSceneLoad();
};

template<class T>
inline void SceneManager::Init()
{
	Scene* pScene = new T();
	currentScene = pScene;
	nextScene = nullptr;
	currentScene->Init();
	currentScene->ExecuteCommands(); // 初期化時に溜まったコマンドを実行
}