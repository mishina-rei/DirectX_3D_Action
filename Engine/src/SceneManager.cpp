#include "Engine_pch.h"

#include "SceneManager.h"
#include "EffekseerManager.h"
#include "Input.h"

SceneManager::SceneManager()
{
}

void SceneManager::Update()
{
	UpdateInput();
	ChangeScene();
	currentScene->Update();
	currentScene->ExecuteCommands(); // 遅延させていたWorldへの変更を適用
}

void SceneManager::Draw()
{
	currentScene->Draw();
}

void SceneManager::Uninit()
{
	currentScene->Uninit();
	delete currentScene;
	currentScene = nullptr;
}

void SceneManager::ChangeScene()
{
	if (nextScene == nullptr)
		return;

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
}
