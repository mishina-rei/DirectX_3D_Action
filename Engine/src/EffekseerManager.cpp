#include "Engine_pch.h"
#include "GraphicsCore.h"
#include "EffekseerManager.h"
#include "CameraSystem.h"
#include <cstdlib>

#if _DEBUG
#pragma comment(lib, "Effekseer/Debug/Effekseer.lib")
#pragma comment(lib, "Effekseer/Debug/EffekseerRendererDX12.lib")
#else
#pragma comment(lib, "Effekseer/Release/Effekseer.lib")
#pragma comment(lib, "Effekseer/Release/EffekseerRendererDX12.lib")
#endif

::Effekseer::ManagerRef EffekseerManager::manager = nullptr;
::Effekseer::RefPtr<EffekseerRenderer::Renderer> EffekseerManager::renderer = nullptr;
std::map<std::string, ::Effekseer::EffectRef> EffekseerManager::effects;

void EffekseerManager::Init()
{
	auto& gfx = GraphicsCore::Get();

	// DX12版に必要なフォーマット情報
	DXGI_FORMAT rtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT dsvFormat = DXGI_FORMAT_D32_FLOAT;
	int swapBufferCount = 2; // ダブルバッファリングなら2

	// 描画用インスタンスの生成
	renderer = ::EffekseerRendererDX12::Create(
		gfx.GetDevice(),
		gfx.GetCommandQueue(),
		swapBufferCount,
		&rtvFormat,
		1,
		dsvFormat,
		false, // ReversedDepthを使用している場合は true
		2000
	);

	// エフェクト管理用インスタンスの生成
	manager = ::Effekseer::Manager::Create(2000);

	// 描画用インスタンスから描画機能を設定
	manager->SetSpriteRenderer(renderer->CreateSpriteRenderer());
	manager->SetRibbonRenderer(renderer->CreateRibbonRenderer());
	manager->SetRingRenderer(renderer->CreateRingRenderer());
	manager->SetTrackRenderer(renderer->CreateTrackRenderer());
	manager->SetModelRenderer(renderer->CreateModelRenderer());

	// テクスチャ・モデル等のローダーを設定
	manager->SetTextureLoader(renderer->CreateTextureLoader());
	manager->SetModelLoader(renderer->CreateModelLoader());
	manager->SetMaterialLoader(renderer->CreateMaterialLoader());

	// 座標系を左手系に設定
	manager->SetCoordinateSystem(Effekseer::CoordinateSystem::LH);

	auto graphicsDevice = renderer->GetGraphicsDevice();
	memoryPool = EffekseerRenderer::CreateSingleFrameMemoryPool(graphicsDevice);
	efkCmdList = EffekseerRenderer::CreateCommandList(graphicsDevice, memoryPool);
}

void EffekseerManager::Uninit()
{
	// エフェクトの破棄
	ClearCache();

	// マネージャーの破棄
	manager.Reset();

	efkCmdList.Reset();
	memoryPool.Reset();

	// レンダラの破棄
	renderer.Reset();
}

void EffekseerManager::ClearCache()
{
	effects.clear();
}

void EffekseerManager::Update()
{
	if (manager.Get())
	{
		manager->Update();
	}
}

void EffekseerManager::Draw()
{
	if (manager == nullptr || renderer == nullptr) return;

	auto& gfx = GraphicsCore::Get();
	// 毎フレーム、現在のコマンドリストをレンダラーに教える
	auto* cmdList = GraphicsCore::Get().GetCommandList();

	////auto efkCmdList = ::EffekseerRenderer::CreateCommandList(gfx.GetDevice(), cmdList);
	//g_renderer->SetCommandList(cmdList);

	// カメラ行列の取得と転置
	DirectX::XMFLOAT4X4 view = CameraSystem::GetView();
	DirectX::XMFLOAT4X4 proj = CameraSystem::GetProjection();

	DirectX::XMMATRIX matView = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&view));
	DirectX::XMMATRIX matProj = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&proj));

	Effekseer::Matrix44 efkView;
	Effekseer::Matrix44 efkProj;

	DirectX::XMStoreFloat4x4(reinterpret_cast<DirectX::XMFLOAT4X4*>(&efkView), matView);
	DirectX::XMStoreFloat4x4(reinterpret_cast<DirectX::XMFLOAT4X4*>(&efkProj), matProj);

	renderer->SetCameraMatrix(efkView);
	renderer->SetProjectionMatrix(efkProj);

	//// 描画開始
	//g_renderer->BeginRendering();
	//g_manager->Draw();
	//g_renderer->EndRendering();

	memoryPool->NewFrame();

	// ネイティブのコマンドリストを、Effekseer用コマンドリストに紐付ける
	EffekseerRendererDX12::BeginCommandList(efkCmdList, cmdList);

	// 紐付けたラッパーを SetCommandList に渡す
	renderer->SetCommandList(efkCmdList);

	// いつも通りの描画処理
	renderer->BeginRendering();
	manager->Draw();
	renderer->EndRendering();

	// コマンドリストの紐付けを解除
	EffekseerRendererDX12::EndCommandList(efkCmdList);
}

Effekseer::Handle EffekseerManager::Play(const char* name, Vector3 position)
{
	if (manager == nullptr) return -1;

	::Effekseer::EffectRef effect = LoadEffect(name);
	if (effect == nullptr) return -1;

	// エフェクト再生
	return manager->Play(effect, position.x, position.y, position.z);
}

void EffekseerManager::Stop(Effekseer::Handle handle)
{
	if (manager.Get()) manager->StopEffect(handle);
}

void EffekseerManager::SetPosition(Effekseer::Handle handle, Vector3 position)
{
	if (manager.Get()) manager->SetLocation(handle, position.x, position.y, position.z);
}

void EffekseerManager::SetRotation(Effekseer::Handle handle, Vector3 rotation)
{
	if (manager.Get()) manager->SetRotation(handle, rotation.x, rotation.y, rotation.z);
}

void EffekseerManager::SetScale(Effekseer::Handle handle, Vector3 scale)
{
	if (manager.Get()) manager->SetScale(handle, scale.x, scale.y, scale.z);
}

::Effekseer::EffectRef EffekseerManager::LoadEffect(const char* path)
{
	// 既に読み込まれているか確認
	if (effects.find(path) != effects.end())
	{
		return effects[path];
	}

	// ワイド文字に変換 (Effekseerはパスにワイド文字を使用)
	wchar_t wPath[256];
	size_t len;
	mbstowcs_s(&len, wPath, 256, path, _TRUNCATE);

	// 読み込み
	::Effekseer::EffectRef effect = ::Effekseer::Effect::Create(manager, (const EFK_CHAR*)wPath);
	if (effect != nullptr)
	{
		effects[path] = effect;
	}
	return effect;
}
