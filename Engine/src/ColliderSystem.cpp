#include "Engine_pch.h"
#include "ColliderSystem.h"
#include <vector>
#include "world.h"
#include "Transform.h"
#include "MeshRenderer.h"
#include "Script.h"
#include "Rigidbody.h"

constexpr int ITER_NUM = 4;                 // 反復処理の回数(処理重かったらなくす)
constexpr float PENETRATION_SLOP = 0.01f;   // 震えるのをふせぐめり込み許容値

ColliderSystem::ColliderSystem():
	m_prevCollisions(),
    tagMap{ true, true, true, true, true,
            true, true, true, true, true,
            true, true, true, true, true,
            true, true, true, true, true,
            true, true, true, true, true, }
{
	SetTagMap(CollisionTag::PLAYER, CollisionTag::BULLET, false);
	SetTagMap(CollisionTag::BULLET, CollisionTag::BULLET, false);
}

void ColliderSystem::Check(ECS::World* world)
{
    std::set<std::pair<ECS::EntityID, ECS::EntityID>> currentCollisions;

    for (int iter = 0; iter < ITER_NUM; ++iter)
    {
        // colliderのデータ全部取り出す
        std::vector<ObbData> obbData;

        world->ForEach<BoxCollider, Transform>([&](ECS::EntityID id, BoxCollider& collider, Transform& transform) {
            ObbData data;
            Vector3 vector;
            Quaternion colliderQuaternion = collider.rotation;
            Quaternion objectQuaternion = transform.rotation;

            // ローカルのx,y,z軸をワールドベクトルにする
            vector = { 1.0f,0.0f,0.0f };
            data.axis.x = (objectQuaternion * colliderQuaternion) * vector;
            vector = { 0.0f,1.0f,0.0f };
            data.axis.y = (objectQuaternion * colliderQuaternion) * vector;
            vector = { 0.0f,0.0f,1.0f };
            data.axis.z = (objectQuaternion * colliderQuaternion) * vector;

            // ワールド座標に変える
            data.pos = transform.position + (objectQuaternion * colliderQuaternion) * collider.center;

            // スケールも移す
            data.scale = collider.size;

			data.tag = collider.tag;

            data.entityId = id;

            obbData.push_back(data);
        });

        Vector3 mtv;                    // 移動させるためのベクトル

        // 全ての組み合わせをチェックする
        for (size_t i = 0; i < obbData.size(); ++i)
        {
            for (size_t j = i + 1; j < obbData.size(); ++j)
            {
                // タグによるフィルタリング
                if (!CheckCollisionTag(obbData[i].tag, obbData[j].tag)) continue;
                
                if (CheckCollisionOBB(obbData[i], obbData[j], &mtv))
                {
                    // 現在のフレームでの衝突ペアを記録 (IDの大小で正規化)
                    ECS::EntityID id1 = obbData[i].entityId;
                    ECS::EntityID id2 = obbData[j].entityId;
                    if (id1 > id2) std::swap(id1, id2);
                    currentCollisions.insert({ id1, id2 });
                    
                    // 衝突応答 (物理的な押し出し)
                    auto& col1 = world->GetComponent<BoxCollider>(obbData[i].entityId);
                    auto& col2 = world->GetComponent<BoxCollider>(obbData[j].entityId);

                    // トリガーなら物理挙動はしない
                    if (!col1.isTrigger && !col2.isTrigger)
                    {
                        auto& trans1 = world->GetComponent<Transform>(obbData[i].entityId);
                        auto& trans2 = world->GetComponent<Transform>(obbData[j].entityId);

                        if (!col1.isStatic && col2.isStatic)
                        {
                            // 1が動的、2が静的 -> 1を動かす
                            trans1.Translate(mtv);
                            obbData[i].pos += mtv;

                            // Rigidbodyがあれば速度を打ち消す
                            if (world->HasComponent<Rigidbody>(obbData[i].entityId)) {
                                auto& rb = world->GetComponent<Rigidbody>(obbData[i].entityId);
                                Vector3 n = mtv.Normalized();
                                float vDot = ColliderSystem::Dot(rb.velocity, n);
                                if (vDot < 0.0f) rb.velocity -= n * vDot;
                            }
                        }
                        else if (col1.isStatic && !col2.isStatic)
                        {
                            // 1が静的、2が動的 -> 2を動かす
                            trans2.Translate(mtv * -1.0f);
                            obbData[j].pos -= mtv;

                            // Rigidbodyがあれば速度を打ち消す
                            if (world->HasComponent<Rigidbody>(obbData[j].entityId)) {
                                auto& rb = world->GetComponent<Rigidbody>(obbData[j].entityId);
                                Vector3 n = (mtv * -1.0f).Normalized();
                                float vDot = ColliderSystem::Dot(rb.velocity, n);
                                if (vDot < 0.0f) rb.velocity -= n * vDot;
                            }
                        }
                        else if (!col1.isStatic && !col2.isStatic)
                        {
                            // 両方動的 -> 半分ずつ動かす
                            trans1.Translate(mtv * 0.5f);
                            obbData[i].pos += mtv * 0.5f;
                            if (world->HasComponent<Rigidbody>(obbData[i].entityId)) {
                                auto& rb = world->GetComponent<Rigidbody>(obbData[i].entityId);
                                // 簡易的なので動的同士の速度解決は省略、あるいは同様に法線方向成分を消す
                            }

                            trans2.Translate(mtv * -0.5f);
                            obbData[j].pos -= mtv * 0.5f;
                        }
                    }
                }
            }
        }
    }

    // イベントの発火 (Enter, Stay)
    for (auto& pair : currentCollisions)
    {
        bool isStay = (m_prevCollisions.find(pair) != m_prevCollisions.end());

        // pair.first のスクリプト呼び出し
        if (world->HasComponent<Script>(pair.first)) {
            auto& scriptComp = world->GetComponent<Script>(pair.first);
            if (scriptComp.script) {
                if (isStay) scriptComp.script->OnCollisionStay(pair.second);
                else        scriptComp.script->OnCollisionEnter(pair.second);
            }
        }
        // pair.second のスクリプト呼び出し
        if (world->HasComponent<Script>(pair.second)) {
            auto& scriptComp = world->GetComponent<Script>(pair.second);
            if (scriptComp.script) {
                if (isStay) scriptComp.script->OnCollisionStay(pair.first);
                else        scriptComp.script->OnCollisionEnter(pair.first);
            }
        }
    }

    // イベントの発火 (Exit)
    for (auto& pair : m_prevCollisions)
    {
        if (currentCollisions.find(pair) == currentCollisions.end())
        {
            // pair.first のスクリプト呼び出し
            if (world->HasComponent<Script>(pair.first)) {
                auto& scriptComp = world->GetComponent<Script>(pair.first);
                if (scriptComp.script) scriptComp.script->OnCollisionExit(pair.second);
            }
            // pair.second のスクリプト呼び出し
            if (world->HasComponent<Script>(pair.second)) {
                auto& scriptComp = world->GetComponent<Script>(pair.second);
                if (scriptComp.script) scriptComp.script->OnCollisionExit(pair.first);
            }
        }
    }
    m_prevCollisions = currentCollisions;
}

bool ColliderSystem::Raycast(ECS::World* world, const Ray& ray, RaycastHit* outHit, float maxDistance, ECS::EntityID ignoreEntity)
{
    float closestT = maxDistance;
    bool hitAnything = false;
    Ray tmpRay = ray;
    tmpRay.direction = ray.direction.Normalized();
    Vector3 bestNormal;
	outHit->hitObj = 0;

    // 現在の全てのOBBデータを取得
    world->ForEach<BoxCollider, Transform>([&](ECS::EntityID id, BoxCollider& collider, Transform& transform) {
        // 無視するエンティティならスキップ
        if (id == ignoreEntity) return;

        // トリガーはレイキャストの対象外にする
        if (collider.isTrigger) return;

        ObbData data;
        Vector3 vector;
        Quaternion colliderQuaternion = collider.rotation;
        Quaternion objectQuaternion = transform.rotation;

        // ローカルのx,y,z軸をワールドベクトルにする
        vector = { 1.0f,0.0f,0.0f };
        data.axis.x = (objectQuaternion * colliderQuaternion) * vector;
        vector = { 0.0f,1.0f,0.0f };
        data.axis.y = (objectQuaternion * colliderQuaternion) * vector;
        vector = { 0.0f,0.0f,1.0f };
        data.axis.z = (objectQuaternion * colliderQuaternion) * vector;

        // ワールド座標に変える
        data.pos = transform.position + (objectQuaternion * colliderQuaternion) * collider.center;

        // スケールも移す
        data.scale = collider.size;

        data.tag = collider.tag;

        float tMin, tMax;
        Vector3 normal;
        if (IntersectRayObb(tmpRay, data, tMin, tMax, normal)) {
            // レイの進行方向で、かつ現在の最短距離より近ければ更新
            if (tMin < closestT && tMin >= 0.0f) {
                closestT = tMin;
                bestNormal = normal;
                outHit->hitObj = id;
                hitAnything = true;
            }
        }
    });

    // 返すデータ作る
    if (hitAnything) {
        outHit->distance = closestT;
        outHit->point = ray.origin + tmpRay.direction * closestT;
        outHit->normal = bestNormal;
    }

    return hitAnything;
}

void ColliderSystem::SetTagMap(CollisionTag tag1, CollisionTag tag2, bool canCollide)
{
    tagMap[static_cast<int>(tag1)][static_cast<int>(tag2)] = canCollide;
    tagMap[static_cast<int>(tag2)][static_cast<int>(tag1)] = canCollide;
};
#ifdef _DEBUG
#endif

double ColliderSystem::GetProjectionRadius(const Vector3 scale, const Vector3 axis, const Axis3 obbAxes)
{
    // 各ローカル軸と分離軸の内積の絶対値 を取り、
    // それをOBBのサイズ（halfExtents）と掛け合わせる
    return
        std::abs(Dot(axis, obbAxes.x)) * scale.x * 0.5f +
        std::abs(Dot(axis, obbAxes.y)) * scale.y * 0.5f +
        std::abs(Dot(axis, obbAxes.z)) * scale.z * 0.5f;
}

bool ColliderSystem::CheckCollisionOBB(ObbData data, ObbData otherData, Vector3* pMtv)
{
    // 中心間のベクトル
    Vector3 T = otherData.pos - data.pos;

    // 軸リスト作る
    Vector3 axisList[15] = {
        data.axis.x,
        data.axis.y,
        data.axis.z,
        otherData.axis.x,
        otherData.axis.y,
        otherData.axis.z
    };

    float minOverlap = FLT_MAX; // 最小の重なり量
    Vector3 mtvAxis;            // 最小の重なりが発生した軸

    // すべての分離軸候補でテストを実行
    // とりあえず6軸でチェック
    for (int i = 0; i < 6; ++i)
    {
        Vector3 L = axisList[i];

        // ゼロベクトルに近い軸はテストをスキップ (ほぼ平行な軸の外積)
        if (L.Magnitude() * L.Magnitude() < FLT_EPSILON) {
            continue;
        }
        L = L.Normalized(); // 軸を正規化

        // 2つのOBBの中心間の距離を、軸Lに射影
        double distance = std::abs(Dot(T, L));

        // 2つのOBBの射影半径の合計
        double r = GetProjectionRadius(data.scale, L, data.axis);
        double otherR = GetProjectionRadius(otherData.scale, L, otherData.axis);

        // 判定の重なり量
        double overlap = r + otherR - distance;

        // 判定
        if (overlap <= 0) {
            // 分離軸が見つかった！ (隙間がある)
            // この時点で衝突していないことが確定
            return false;
        }

        // 最も小さい重なり量とその時の軸を記録する
        if (minOverlap > overlap)
        {
            minOverlap = overlap;
            mtvAxis = L;
        }
    }

    int idx = 6;
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            axisList[idx] = Cross(axisList[i], axisList[j + 3]);
            idx++;
        }
    }

    for (int i = 6; i < 15; ++i)
    {
        Vector3 L = axisList[i];

        // ゼロベクトルに近い軸はテストをスキップ (ほぼ平行な軸の外積)
        if (L.Magnitude() * L.Magnitude() < FLT_EPSILON) {
            continue;
        }
        L = L.Normalized(); // 軸を正規化

        // 2つのOBBの中心間の距離を、軸Lに射影
        double distance = std::abs(Dot(T, L));

        // 2つのOBBの射影半径の合計
        double r = GetProjectionRadius(data.scale, L, data.axis);
        double otherR = GetProjectionRadius(otherData.scale, L, otherData.axis);

        // 判定の重なり量
        double overlap = r + otherR - distance;

        // 判定
        if (overlap <= 0) {
            // 分離軸が見つかった(隙間がある)
            // この時点で衝突していないことが確定
            return false;
        }

        // 最も小さい重なり量とその時の軸を記録する
        if (minOverlap > overlap)
        {
            minOverlap = overlap;
            mtvAxis = L;
        }
    }

    // めり込み許容値を適用
    minOverlap = std::max<float>(0.0f, minOverlap - PENETRATION_SLOP);

    // MTVの計算
    *pMtv = mtvAxis * minOverlap;
        
    // MTVの方向を補正する
    // MTVは「AをBから押し出す」方向でなければならない。
    // AからBへのベクトル(T)と、mtvAxis の向きが逆（内積が負）なら、MTVを反転させる
    if (Dot(T, mtvAxis) >= 0) {
        *pMtv = *pMtv * -1.0;
    }
    // 15軸すべてをテストしたが、分離軸は一つも見つからなかった
    // したがって、衝突している
    return true;
}

bool ColliderSystem::IntersectRayObb(const Ray& ray, const ObbData& obb, float& tMin, float& tMax, Vector3& outNormal) 
{
    tMin = -FLT_MAX;      // レイの開始位置
    tMax = FLT_MAX;       // レイの最大射程

    Vector3 p = obb.pos - ray.origin; // OBB中心へのベクトル
    float half[3] = { obb.scale.x * 0.5f,obb.scale.y * 0.5f,obb.scale.z * 0.5f };     // ハーフエキステント（半辺長）

    // OBBの3軸（x, y, z）に対してループ
    Vector3 axes[3] = { obb.axis.x, obb.axis.y, obb.axis.z };

    for (int i = 0; i < 3; ++i) 
    {
        float e = ColliderSystem::Dot(axes[i], p); // スラブ中心までの距離
        float f = ColliderSystem::Dot(axes[i], ray.direction); // レイの方向成分

        if (std::abs(f) > 0.0001f) // レイがスラブと平行でない場合
        { 
            float inTime = (e + half[i]) / f; // ここでのh.xは実際にはaxes[i]に対応するhの成分
            float outTime = (e - half[i]) / f;

            if (inTime > outTime) std::swap(inTime, outTime);

            // tMinの更新（最も遅い進入時間）
            if (inTime > tMin) {    
                tMin = inTime;
                // 法線の記録（どの面から進入したか）
                outNormal = axes[i] * (f > 0.0f ? -1.0f : 1.0f);
            }
            // tMaxの更新（最も早い退出時間）
            if (outTime < tMax) tMax = outTime;

            if (tMin > tMax) return false; // 重なりがない
        }
        else {
            // レイがスラブと平行な場合、始点がスラブ内にないと当たらない
            if (-e - half[i] > 0.0f || -e + half[i] < 0.0f) return false;
        }
    }
    return true;
}

bool ColliderSystem::CheckCollisionTag(CollisionTag tag1, CollisionTag tag2)
{
    return tagMap[static_cast<int>(tag1)][static_cast<int>(tag2)];
}

float ColliderSystem::Dot(Vector3 v, Vector3 other)
{
    return v.x * other.x + v.y * other.y + v.z * other.z;
}

Vector3 ColliderSystem::Cross(Vector3 v, Vector3 other)
{
    return { v.y * other.z - v.z * other.y, v.z * other.x - v.x * other.z,v.x * other.y - v.y * other.x };
}