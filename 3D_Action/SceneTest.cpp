#include "Engine_pch.h"
#include "SceneTest.h"
#include "Transform.h"
#include "Camera.h"
#include "MeshRenderer.h"
#include "SpriteRenderer.h"
#include "CameraSystem.h"
#include "Input.h"

void SceneTest::Init()
{
    // ==========================================
    // 1. 使用するコンポーネントの構造的登録
    // ==========================================
    RegisterStructural<Transform>();
    RegisterStructural<Camera>();
    RegisterStructural<MeshRenderer>();
    RegisterStructural<SpriteRenderer>();

    // ==========================================
    // 2. カメラエンティティの作成
    // ==========================================
    auto camera = CreateEntity();
    Transform camTrans;
    camTrans.position = { 0.0f, 3.0f, -10.0f }; // 少し高くて後ろの位置
    camTrans.rotation = Quaternion::FromRotation(15.0f, 0.0f, 0.0f); // 少し下を見下ろす

    AddComponent(camera, camTrans);
    AddComponent(camera, Camera{}); // デフォルト設定のカメラ
    CameraSystem::SetCamera(camera); // メインカメラとして設定

    // ==========================================
    // 3. 3Dモデルエンティティの作成
    // ==========================================
    auto player = CreateEntity();
    Transform playerTrans;
    playerTrans.position = { 0.0f, 0.0f, 0.0f }; // 原点
    // FBXのスケールが大きすぎることが多いので、最初は小さめにしておくと安心です
    playerTrans.scale = { 0.01f, 0.01f, 0.01f };

    MeshRenderer meshRenderer;
    // ※ ここは実際のモデルファイルのパスに書き換えてください！
    meshRenderer.pModel->CreateFromFile("Assets/Model/player.fbx");

    AddComponent(player, playerTrans);
    AddComponent(player, meshRenderer);

    // ==========================================
    // 4. 2Dスプライト(UI)エンティティの作成
    // ==========================================
    auto ui = CreateEntity();
    Transform uiTrans;
    uiTrans.position = { 100.0f, 100.0f, 0.0f }; // 画面左上から(100, 100)の位置
    uiTrans.scale = { 1.0f, 1.0f, 1.0f };

    SpriteRenderer spriteRenderer;
    // ※ ここも実際のテクスチャファイルのパスに書き換えてください！
    spriteRenderer.SetTexture("Assets/Texture/ui_test.png");
    spriteRenderer.isUI = true; // UIモードを有効化

    AddComponent(ui, uiTrans);
    AddComponent(ui, spriteRenderer);
}

void SceneTest::Update()
{
    // Sceneの基底クラスのUpdateを必ず呼ぶ (システム群の実行など)
    Scene::Update();

    // デバッグ用に、スペースキーで3Dモデルを回転させてみるテスト
    if (IsKeyPress(VK_SPACE))
    {
        // 最初のエンティティ(おそらくplayer)を取得して回す簡易処理
        // 本来はPlayerScriptなどのコンポーネントで行います
        ForEachComponent<Transform>([](ECS::EntityID id, Transform& t) {
            // 原点付近にいるオブジェクトだけ回す
            if (t.position.Magnitude() < 1.0f) {
                t.Rotate(Quaternion::FromRotation(0.0f, 1.0f, 0.0f));
            }
            });
    }
}

void SceneTest::Uninit()
{
    // 基底クラスのUninit
    Scene::Uninit();
}