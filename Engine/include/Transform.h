#pragma once

#include "Vector.h"
#include "Quaternion.h"

struct Transform
{
	Vector3 position;		// 座標
	Quaternion rotation;	// 回転
	Vector3 scale;			// 拡縮

	Transform()
		: position(0.0f, 0.0f, 0.0f)
		, rotation(Quaternion::Identity())
		, scale(1.0f, 1.0f, 1.0f)
	{}

	template<class Archive>
	void Reflect(Archive& archive) {
		archive.Property("Position", position);
		archive.Property("Rotation", rotation);
		archive.Property("Scale", scale);
	}

	// 回転させる
	void Rotate(Quaternion q)
	{
		rotation *= q;
		rotation = rotation.Normalize();
	}

	// 移動させる
	void Translate(const Vector3& v)
	{
		position += v;
	}
};