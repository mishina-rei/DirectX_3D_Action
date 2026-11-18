#include "Engine_pch.h"
#include "SceneTest.h"
#include "Transform.h"
#include "Camera.h"
#include "MeshRenderer.h"
#include "SpriteRenderer.h"
#include "CameraSystem.h"
#include "Input.h"
#include "Name.h"
#include "UUID.h"
#include "Collider.h"
#include "Rigidbody.h"
#include "Script.h"
#include "MoveTestScript.h"
#include "SceneTransitionTestScript.h"
#include "ScriptRegistry.h"
#include "SceneSerializer.h"
#if WITH_EDITOR
#include "EditorUI.h"
#endif

#include <fstream>
#include <stdexcept>

namespace
{
    // 保存形式の登録は共通で行い、インスペクターの登録だけをエディタに限定する
    template<typename T>
    void RegisterSceneComponent(Scene& scene, const std::string& name, bool canRemove = true)
    {
        scene.RegisterStructural<T>();
        SceneSerializer::RegisterComponent<T>(name);
#if WITH_EDITOR
        EditorUI::Get().RegisterComponent<T>(name, canRemove);
#endif
    }
}

void SceneTest::Init()
{
    ScriptRegistry::Get().Register<MoveTestScript>("MoveTestScript");
    ScriptRegistry::Get().Register<SceneTransitionTestScript>("SceneTransitionTestScript");

    RegisterSceneComponent<Name>(*this, "Name Component", false);
    RegisterSceneComponent<Transform>(*this, "Transform", false);
    RegisterSceneComponent<UUIDComponent>(*this, "UUID", false);
    RegisterSceneComponent<Camera>(*this, "Camera");
    RegisterSceneComponent<MeshRenderer>(*this, "MeshRenderer");
    RegisterSceneComponent<SpriteRenderer>(*this, "SpriteRenderer");
    RegisterSceneComponent<BoxCollider>(*this, "BoxCollider");
    RegisterSceneComponent<Rigidbody>(*this, "Rigidbody");
    RegisterSceneComponent<Script>(*this, "Script");

    // エディタとゲーム本編で同じシェーダーを使用する
#if WITH_EDITOR
    ShaderManager::Get().LoadShader("VS_Standard", L"src/Shader/StandardVS.hlsl", L"main", L"vs_6_0");
    ShaderManager::Get().LoadShader("PS_Standard", L"src/Shader/StandardPS.hlsl", L"main", L"ps_6_0");
#else
    ShaderManager::Get().LoadShader("VS_Standard", L"Shader/StandardVS.hlsl", L"main", L"vs_6_0");
    ShaderManager::Get().LoadShader("PS_Standard", L"Shader/StandardPS.hlsl", L"main", L"ps_6_0");
#endif

#if WITH_EDITOR
    // 配置と設定はシーンファイルから復元する
    EditorUI::Get().LoadStartupScene(GetWorld());
#else
    // ゲームの開始シーンは、最後に編集したシーンとは別に指定する
    std::ifstream file("GameSettings.json");
    if (!file)
    {
        throw std::runtime_error("Cannot open GameSettings.json.");
    }

    const json settings = json::parse(file, nullptr, false);
    if (!settings.is_object() || !settings.contains("StartupScene") ||
        !settings["StartupScene"].is_string())
    {
        throw std::runtime_error("Invalid StartupScene in GameSettings.json.");
    }

    // 通常のシーン遷移と同じ読み込み処理で復元し、最初の更新から実行する
    const std::string startupScenePath = settings["StartupScene"].get<std::string>();
    if (!SceneManager::GetInstance().RequestSceneLoad(startupScenePath))
    {
        throw std::runtime_error("Cannot request the game startup scene.");
    }
#endif
}
