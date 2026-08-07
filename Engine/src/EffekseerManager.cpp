#include "Engine_pch.h"

#include "EffekseerManager.h"
#include "CameraSystem.h"
#include <cstdlib>

#if _DEBUG
#pragma comment(lib, "Effekseer/Debug/Effekseer.lib")
#pragma comment(lib, "Effekseer/Debug/EffekseerRendererDX11.lib")
#else
#pragma comment(lib, "Effekseer/Release/Effekseer.lib")
#pragma comment(lib, "Effekseer/Release/EffekseerRendererDX11.lib")
#endif

::Effekseer::ManagerRef EffekseerManager::g_manager = nullptr;
::EffekseerRendererDX11::RendererRef EffekseerManager::g_renderer = nullptr;
std::map<std::string, ::Effekseer::EffectRef> EffekseerManager::g_effects;

void EffekseerManager::Init()
{
	// 描画用インスタンスの生成
	g_renderer = ::EffekseerRendererDX11::Renderer::Create(GetDevice(), GetContext(), 2000);

	// エフェクト管理用インスタンスの生成
	g_manager = ::Effekseer::Manager::Create(2000);

	// 描画用インスタンスから描画機能を設定
	g_manager->SetSpriteRenderer(g_renderer->CreateSpriteRenderer());
	g_manager->SetRibbonRenderer(g_renderer->CreateRibbonRenderer());
	g_manager->SetRingRenderer(g_renderer->CreateRingRenderer());
	g_manager->SetTrackRenderer(g_renderer->CreateTrackRenderer());
	g_manager->SetModelRenderer(g_renderer->CreateModelRenderer());

	// 描画用インスタンスはテクスチャの読み込みを行うローダーを持つため、それを設定する
	g_manager->SetTextureLoader(g_renderer->CreateTextureLoader());
	g_manager->SetModelLoader(g_renderer->CreateModelLoader());
	g_manager->SetMaterialLoader(g_renderer->CreateMaterialLoader());

	// 座標系を左手系に設定 (DirectX11)
	g_manager->SetCoordinateSystem(Effekseer::CoordinateSystem::LH);
}

void EffekseerManager::Uninit()
{
	// エフェクトの破棄
	ClearCache();

	// マネージャーの破棄
	g_manager.Reset();

	// レンダラの破棄
	g_renderer.Reset();
}

void EffekseerManager::ClearCache()
{
	g_effects.clear();
}

void EffekseerManager::Update()
{
	if (g_manager.Get())
	{
		g_manager->Update();
	}
}

void EffekseerManager::Draw()
{
	if (g_manager == nullptr || g_renderer == nullptr) return;

	// カメラ行列の取得
	DirectX::XMFLOAT4X4 view = CameraSystem::GetView();
	DirectX::XMFLOAT4X4 proj = CameraSystem::GetProjection();

	// 行列を設定
	// CameraSystemから取得した行列はHLSL用に転置(Transpose)されているため、
	// Effekseerに渡す前にもう一度転置して元に戻す必要がある
	DirectX::XMMATRIX matView = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&view));
	DirectX::XMMATRIX matProj = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&proj));

	Effekseer::Matrix44 efkView;
	Effekseer::Matrix44 efkProj;

	DirectX::XMStoreFloat4x4(reinterpret_cast<DirectX::XMFLOAT4X4*>(&efkView), matView);
	DirectX::XMStoreFloat4x4(reinterpret_cast<DirectX::XMFLOAT4X4*>(&efkProj), matProj);

	g_renderer->SetCameraMatrix(efkView);
	g_renderer->SetProjectionMatrix(efkProj);

	// 描画開始
	g_renderer->BeginRendering();

	// 描画
	g_manager->Draw();

	// 描画終了
	g_renderer->EndRendering();
}

Effekseer::Handle EffekseerManager::Play(const char* name, Vector3 position)
{
	if (g_manager == nullptr) return -1;

	::Effekseer::EffectRef effect = LoadEffect(name);
	if (effect == nullptr) return -1;

	// エフェクト再生
	return g_manager->Play(effect, position.x, position.y, position.z);
}

void EffekseerManager::Stop(Effekseer::Handle handle)
{
	if (g_manager.Get()) g_manager->StopEffect(handle);
}

void EffekseerManager::SetPosition(Effekseer::Handle handle, Vector3 position)
{
	if (g_manager.Get()) g_manager->SetLocation(handle, position.x, position.y, position.z);
}

void EffekseerManager::SetRotation(Effekseer::Handle handle, Vector3 rotation)
{
	if (g_manager.Get()) g_manager->SetRotation(handle, rotation.x, rotation.y, rotation.z);
}

void EffekseerManager::SetScale(Effekseer::Handle handle, Vector3 scale)
{
	if (g_manager.Get()) g_manager->SetScale(handle, scale.x, scale.y, scale.z);
}

::Effekseer::EffectRef EffekseerManager::LoadEffect(const char* path)
{
	// 既に読み込まれているか確認
	if (g_effects.find(path) != g_effects.end())
	{
		return g_effects[path];
	}

	// ワイド文字に変換 (Effekseerはパスにワイド文字を使用)
	wchar_t wPath[256];
	size_t len;
	mbstowcs_s(&len, wPath, 256, path, _TRUNCATE);

	// 読み込み
	::Effekseer::EffectRef effect = ::Effekseer::Effect::Create(g_manager, (const EFK_CHAR*)wPath);
	if (effect != nullptr)
	{
		g_effects[path] = effect;
	}
	return effect;
}
