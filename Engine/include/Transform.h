#pragma once

#include "Vector.h"
#include "Quaternion.h"
#include "Archive.h"
#include "world.h"

struct Transform
{
	Vector3 position;		// 座標
	Quaternion rotation;	// 回転
	Vector3 scale;			// 拡縮
	EntityRef parent;		// 親への参照

	Transform()
		: position(0.0f, 0.0f, 0.0f)
		, rotation(Quaternion::Identity())
		, scale(1.0f, 1.0f, 1.0f)
		, parent()
	{}

	template<class Archive>
	void Reflect(Archive& archive) {
		archive.Property("Position", position);
		archive.Property("Rotation", rotation);
		archive.Property("Scale", scale);

		EntityRef oldParent = parent;
		archive.Property("Parent", parent);

		// エディタ操作（ImGuiArchive）で親が変わった瞬間に、見た目を維持するようにローカル値を自動調整！
		if constexpr (std::is_same_v<Archive, ImGuiArchive>) {
			if (archive.world && oldParent.id != parent.id) {
				EntityRef newParent = parent;
				parent = oldParent;
				SetParent(archive.world, newParent, true);
			}
		}
	}

	// 親の設定（見た目のワールド変換を維持するかどうか）
	void SetParent(ECS::World* world, EntityRef newParent, bool keepWorldTransform = true)
	{
		if (parent.id == newParent.id) return;

		if (keepWorldTransform && world)
		{
			// 1. 変更前のワールド行列を取得
			DirectX::XMMATRIX oldWorld = GetWorldMatrix(world);

			// 2. 新しい親のローカル座標系へと変換 (OldWorld * InvNewParent)
			DirectX::XMMATRIX newLocal = oldWorld;
			if (newParent.id != ECS::INVALID_ENTITY_ID && world->HasComponent<Transform>(newParent.id))
			{
				DirectX::XMMATRIX parentWorld = world->GetComponent<Transform>(newParent.id).GetWorldMatrix(world);
				DirectX::XMVECTOR det;
				DirectX::XMMATRIX invParent = DirectX::XMMatrixInverse(&det, parentWorld);
				newLocal = oldWorld * invParent;
			}

			// 3. 行列から Scale, Rotation, Translation を分解して代入
			DirectX::XMVECTOR s, r, t;
			if (DirectX::XMMatrixDecompose(&s, &r, &t, newLocal))
			{
				position = t;
				rotation = r;
				scale = s;
			}
		}

		parent = newParent;
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

	// ローカル行列の取得
	DirectX::XMMATRIX GetLocalMatrix() const
	{
		DirectX::XMMATRIX S = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);
		DirectX::XMMATRIX R = DirectX::XMMatrixRotationQuaternion(rotation);
		DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(position.x, position.y, position.z);
		return S * R * T;
	}

	// ワールド行列の取得（親のワールド行列を再帰的に合成）
	DirectX::XMMATRIX GetWorldMatrix(ECS::World* world, int depth = 0) const
	{
		DirectX::XMMATRIX local = GetLocalMatrix();

		// 親が存在し、循環参照などの無限ループを防ぐため深さ32まで
		if (depth < 32 && parent.id != ECS::INVALID_ENTITY_ID && world)
		{
			if (world->HasComponent<Transform>(parent.id))
			{
				auto& parentTrans = world->GetComponent<Transform>(parent.id);
				return local * parentTrans.GetWorldMatrix(world, depth + 1);
			}
		}

		return local;
	}
};