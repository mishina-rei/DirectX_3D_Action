#pragma once
#include "world.h"
#include "Camera.h"
#include <DirectXMath.h>

#include <d3d11.h>
#include <wrl/client.h> // ComPtr用


using namespace Microsoft::WRL;

class RenderSystem
{
public:

	static HRESULT Init();
	// 描画処理
	// view, proj: カメラのビュー・プロジェクション行列
	static void Draw(ECS::World* world);

private:

	static ComPtr<ID3D11Texture2D> m_pShadowMapTexture;       // シャドウマップ用テクスチャ本体
	static ComPtr<ID3D11DepthStencilView> m_pShadowMapDSV;    // 深度バッファ用
	static ComPtr<ID3D11ShaderResourceView> m_pShadowMapSRV;  // シェーダーリソース用
	static D3D11_VIEWPORT m_ShadowViewport;                   // シャドウマップ用ビューポート
	static ComPtr<ID3D11Buffer> m_pShadowCB;                  // シャドウ用定数バッファ

	static Camera light;
};