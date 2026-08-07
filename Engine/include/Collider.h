#pragma once

#include "Vector.h"
#include "Quaternion.h"
#include "EntityID.h"
#include <string>

// 衝突判定用のタグ定義
enum class CollisionTag
{
    Default,
    NONE,
    PLAYER,
    ENEMY,
    BULLET,

    // 必要に応じて追加
    COLLISION_TAG_MAX
};

struct BoxCollider
{
	Vector3 center = { 0.0f, 0.0f, 0.0f };	    // ローカル座標からのオフセット
	Vector3 size = { 1.0f, 1.0f, 1.0f };	    // Boxのサイズ
    Quaternion rotation = Quaternion::Identity();	// ローカル座標の回転
	bool isTrigger = false;					    // 物理演算はせず衝突イベントのみ発生させるか
    bool isStatic = true;                       // 静的オブジェクト（動かないオブジェクト）か
    CollisionTag tag = CollisionTag::NONE;   // 識別用タグ

    BoxCollider(){}

    BoxCollider(Vector3 _center, Vector3 _size, Quaternion _rotation, bool _isTrigger = false, CollisionTag _tag = CollisionTag::Default):
        center(_center), size(_size), rotation(_rotation), isTrigger(_isTrigger), tag(_tag)
    {
    }

    void Rotate(Quaternion q)
	{
		rotation *= q;
		rotation = rotation.Normalize();
	}
};

// 衝突判定計算用の構造体
struct Axis3
{
    Vector3 x;
    Vector3 y;
    Vector3 z;
};

struct ObbData
{
    Vector3 pos;
    Vector3 scale;
    Axis3 axis;
    ECS::EntityID entityId;
    CollisionTag tag;
};