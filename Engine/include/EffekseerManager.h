#pragma once
#include <Effekseer/Effekseer.h>
#include <Effekseer/EffekseerRendererDX11.h>
#include "Vector.h"
#include <map>
#include <string>

class EffekseerManager
{
public:
	static void Init();
	static void Uninit();
	static void Update();
	static void Draw();
	static void ClearCache();

	/**
	 * @brief   エフェクトを再生する
	 * @param   name エフェクトファイルのパス (例: "Assets/Effect/test.efk")
	 * @param   position 再生座標
	 * @return  再生ハンドル
	 */
	static Effekseer::Handle Play(const char* name, Vector3 position);
	
	static void Stop(Effekseer::Handle handle);
	static void SetPosition(Effekseer::Handle handle, Vector3 position);
	static void SetRotation(Effekseer::Handle handle, Vector3 rotation);
	static void SetScale(Effekseer::Handle handle, Vector3 scale);

private:
	static ::Effekseer::ManagerRef g_manager;
	static ::EffekseerRendererDX11::RendererRef g_renderer;
	static std::map<std::string, ::Effekseer::EffectRef> g_effects;

	static ::Effekseer::EffectRef LoadEffect(const char* path);
};
