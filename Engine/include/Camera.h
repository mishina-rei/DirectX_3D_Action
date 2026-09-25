#pragma once

#include "Vector.h"
#include "Quaternion.h"
#include "Defines.h"
#include <DirectXMath.h>

struct Camera
{
	float fov = 90.0f;
	float aspect = (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT;
	float nearClip = 0.1f;
	float farClip = 1000.0f;

	template<class Archive>
	void Reflect(Archive& archive) {
		archive.Property("FOV", fov);
		archive.Property("NearClip", nearClip);
		archive.Property("FarClip", farClip);
	}
    
	// 射影行列の取得
	DirectX::XMFLOAT4X4 GetProjectionMatrix() const
	{
		DirectX::XMFLOAT4X4 mat;
		DirectX::XMStoreFloat4x4(&mat, DirectX::XMMatrixTranspose(
			DirectX::XMMatrixPerspectiveFovLH(
			DirectX::XMConvertToRadians(fov),
			aspect,
			nearClip,
			farClip
		)));
		DirectX::XMStoreFloat4x4(&mat,
			DirectX::XMMatrixPerspectiveFovLH(
				DirectX::XMConvertToRadians(fov),
				aspect,
				nearClip,
				farClip
			));
		return mat;
	}

	// ビュー行列の取得 (Transformを使用)
	DirectX::XMFLOAT4X4 GetViewMatrix(const Vector3& position, const Quaternion& rotation) const
	{
		// 回転から前方ベクトルと上方向ベクトルを算出
		Vector3 forward = Vector3(0.0f, 0.0f, 1.0f);
		forward = rotation * forward;

		Vector3 up = Vector3(0.0f, 1.0f, 0.0f);
		up = rotation * up;

		DirectX::XMFLOAT4X4 mat;
		DirectX::XMStoreFloat4x4(&mat, DirectX::XMMatrixTranspose(DirectX::XMMatrixLookToLH(position, forward, up)));
		DirectX::XMStoreFloat4x4(&mat, DirectX::XMMatrixLookToLH(position, forward, up));
		return mat;
    }

	// ビュー行列の取得 (LookAt指定)
	DirectX::XMFLOAT4X4 GetViewMatrixLookAt(const Vector3& position, const Vector3& target, const Vector3& up = Vector3(0.0f, 1.0f, 0.0f)) const
	{
		DirectX::XMFLOAT4X4 mat;
		DirectX::XMStoreFloat4x4(&mat, DirectX::XMMatrixTranspose(
			DirectX::XMMatrixLookAtLH(position, target, up)
		));
		DirectX::XMStoreFloat4x4(&mat,
			DirectX::XMMatrixLookAtLH(position, target, up)
		);
		return mat;
	}
};
