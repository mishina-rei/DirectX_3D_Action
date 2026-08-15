// SceneManager.h

#pragma once

#include "Scene.h"

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
	void SetScene() { nextScene = new T(); }

private:

	SceneManager();

	// シーン変える
	void ChangeScene();


	Scene* currentScene;
	Scene* nextScene;
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