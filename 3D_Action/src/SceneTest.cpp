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
    RegisterStructural<Transform>();
    RegisterStructural<Camera>();
    RegisterStructural<MeshRenderer>();
    RegisterStructural<SpriteRenderer>();

    ShaderManager::Get().LoadShader("VS_Standard", L"src\\Shader\\StandardVS.hlsl", L"main", L"vs_6_0");
    ShaderManager::Get().LoadShader("PS_Standard", L"src\\Shader\\StandardPS.hlsl", L"main", L"ps_6_0");

    // カメラの作成
    auto camera = CreateEntity();
    Transform camTrans;
    camTrans.position = { 0.0f, 3.0f, -6.0f }; // 少し高くて後ろの位置
    camTrans.rotation = Quaternion::FromRotation(15.0f, 0.0f, 0.0f); // 少し下を見下ろす

    AddComponent(camera, camTrans);
    AddComponent(camera, Camera{}); // デフォルト設定のカメラ
    CameraSystem::SetCamera(camera); // メインカメラとして設定

    // 3Dモデルエンティティの作成
    auto player = CreateEntity();
    Transform playerTrans;
    playerTrans.position = { 0.0f, 0.0f, 0.0f }; // 原点
	playerTrans.rotation = Quaternion::FromRotation(0.0f, 130.0f, 0.0f); // Y軸回転で180度回転（前を向く）

    MeshRenderer meshRenderer;

    //// 2Dスプライト(UI)エンティティの作成
    auto ui = CreateEntity();
    Transform uiTrans;
    uiTrans.position = { 100.0f, 100.0f, 0.0f }; // 画面左上から(100, 100)の位置
    uiTrans.scale = { 1.0f, 1.0f, 1.0f };

    SpriteRenderer spriteRenderer;
    spriteRenderer.isUI = true; // UIモードを有効化


    auto& gfx = GraphicsCore::Get();
    auto ictx = gfx.GetCommandContext();

    ictx.BeginFrame(gfx.GetCurrentCommandAllocator());
    
    meshRenderer.pModel->CreateFromFile("Assets\\voletir.fbx");

    spriteRenderer.SetTexture("Assets/Texture/T_Voletir_Diffuse.png");

    ictx.EndFrame();

    gfx.FlushCommandQueue();

    meshRenderer.pModel->FreeUploadBuffers();
	spriteRenderer.pTexture->FreeUploadBuffer();


	meshRenderer.material = std::make_shared<Material>("VS_Standard", "PS_Standard");

    AddComponent(player, playerTrans);
    AddComponent(player, meshRenderer);


    AddComponent(ui, uiTrans);
    AddComponent(ui, spriteRenderer);
}