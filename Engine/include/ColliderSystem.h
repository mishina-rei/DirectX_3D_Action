/*****************************************************************//**
 * @file   ColliderSystem.h
 * @brief  当たり判定コンポーネント(回転させれる直方体のみ)を管理する
 *
 * @author 三品怜
 * @date   2025/11/16
 *********************************************************************/
#pragma once

#include "Collider.h"
#include "EntityID.h"
#include <iostream>
#include <set>

namespace ECS { class World; }

 /**
  * @brief 全ての当たり判定を管理するシステム
  */
class ColliderSystem
{
public:

	// レイ用構造体
	struct Ray {
		Vector3 origin;
		Vector3 direction;
	};

	// レイが当たった時の情報
	struct RaycastHit {
		Vector3 point;    // 当たった座標
		float distance;   // 距離
		Vector3 normal;   // 当たった面の法線
		ECS::EntityID hitObj;// 当たったオブジェクト
	};

	/**
	 * @brief 全ての当たり判定コンポーネントを当たってるかチェック
	 * @param world ECSのワールド
	 */
	void Check(ECS::World* world);

	/**
	 * @brief 全ての当たり判定コンポーネントとレイが当たってるかチェック
	 * @param world ECSのワールド
	 * @param ray チェックするレイ
	 * @param RaycastHit 当たった時に情報を入れる構造体のポインター
	 * @param maxDistance レイの長さ
	 */
	bool Raycast(ECS::World* world, const Ray& ray, RaycastHit* outHit, float maxDistance, ECS::EntityID ignoreEntity = -1);

	void SetTagMap(CollisionTag tag1, CollisionTag tag2, bool canCollide);
    
private:

	ColliderSystem();

	/**
	 * @brief 軸に対する射影半径を計算する
	 * @param Vector3 当たり判定の大きさ
	 * @param Vector3 射影する軸
	 * @param Collider::Axis3 当たり判定のx,y,z軸のベクトル
	 * @return Vector3
	 */
	double GetProjectionRadius(const Vector3 scale, const Vector3 axis, const Axis3 obbAxes);

	/**
	 * @brief OBB同士の衝突判定 (SAT)
	 * @param a OBB A
	 * @param b OBB B
	 * @return 衝突していれば true
	 */
	bool CheckCollisionOBB(ObbData data, ObbData otherData, Vector3* pMtv);

	/**
	 * @brief OBBとrayの当たり判定
	 * @return 衝突していれば true
	 */
	bool IntersectRayObb(const Ray& ray, const ObbData& obb, float& tMin, float& tMax, Vector3& outNormal);

	/**
	 * @brief タグによる衝突判定のフィルタリング
	 * @param tag1 タグ1
	 * @param tag2 タグ2
	 * @return 衝突判定を行う場合はtrue
	 */
	bool CheckCollisionTag(CollisionTag tag1, CollisionTag tag2);

public:

	/**
	 * @brief Vector3の内積
	 * @param Vector3
	 * @param Vector3
	 * @return Vector3 内積
	 */
	float Dot(Vector3 v, Vector3 other);

	/**
	 * @brief Vector3の外積
	 * @param Vector3
	 * @param Vector3
	 * @return Vector3 外積
	 */
	Vector3 Cross(Vector3 v, Vector3 other);

private:
	std::set<std::pair<ECS::EntityID, ECS::EntityID>> m_prevCollisions;

	bool tagMap[static_cast<int>(CollisionTag::COLLISION_TAG_MAX)][static_cast<int>(CollisionTag::COLLISION_TAG_MAX)];

public:

	/**
	 * @brief 唯一のインスタンスを取得する
	 * @return RenderSystemインスタンスへの参照
	 */
	static ColliderSystem& Instance()
	{
		static ColliderSystem s_instance;
		return s_instance;
	}
};