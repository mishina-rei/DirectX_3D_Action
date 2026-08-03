#include "DirectX12_pch.h"

#include "Animator.h"

// 現在の時間に対応するキーフレームのインデックスを探す
int FindPositionIndex(float animationTime, const BoneAnimationTrack& track) {
    for (size_t i = 0; i < track.positions.size() - 1; ++i) {
        if (animationTime < track.positions[i + 1].timeStamp) {
            return static_cast<int>(i);
        }
    }
    return 0; // 見つからなければ最初のフレーム
}

int FindRotationIndex(float animationTime, const BoneAnimationTrack& track) {
    for (size_t i = 0; i < track.rotations.size() - 1; ++i) {
        if (animationTime < track.rotations[i + 1].timeStamp) {
            return static_cast<int>(i);
        }
    }
    return 0;
}

int FindScalingIndex(float animationTime, const BoneAnimationTrack& track) {
    for (size_t i = 0; i < track.scales.size() - 1; ++i) {
        if (animationTime < track.scales[i + 1].timeStamp) {
            return static_cast<int>(i);
        }
    }
    return 0;
}

void Animator::Initialize(const AnimationClip* clip, const NodeData* rootNode, const std::unordered_map<std::string, BoneInfo>* boneInfoMap) {
    this->currentClip = clip;
    this->rootNode = rootNode;
    this->boneInfoMap = boneInfoMap;
    this->currentTime = 0.0f;

    // GPUに送る行列配列をリサイズ（ボーンの総数分。最大100〜256個程度）
    //finalBoneMatrices.resize(boneInfoMap->size(), DirectX::XMMatrixIdentity());
    finalBoneMatrices.resize(256, DirectX::XMMatrixIdentity());
}

void Animator::UpdateAnimation(float dt) {
    if (!currentClip || !rootNode) return;

    // 時間を進める (TicksPerSecond を掛けてアニメーションの基準時間に合わせる)
    currentTime += dt * currentClip->ticksPerSecond;

    // ループ再生させるために剰余をとる
    currentTime = fmod(currentTime, currentClip->duration);

    // ルートノードから階層計算をスタート (初期の親行列は単位行列)
    CalculateBoneTransform(rootNode, DirectX::XMMatrixIdentity());

    //if (!finalBoneMatrices.empty()) {
    //    static float debugAngle = 0.0f;
    //    debugAngle += dt * 5.0f;  
    //    finalBoneMatrices[0] = DirectX::XMMatrixRotationY(debugAngle);
    //}

    // ▼▼▼ 追加：テスト用強制上書き ▼▼▼
    //for (int i = 0; i < 100; ++i) {
    //    // 全ボーンを強制的に単位行列にする（Transposeしても単位行列は同じなのでそのまま）
    //    finalBoneMatrices[i] = DirectX::XMMatrixIdentity();
    //}
}

void Animator::CalculateBoneTransform(const NodeData* node, DirectX::XMMATRIX parentTransform) {
    std::string nodeName = node->name;

    // 基本はノードが元々持っている初期姿勢(ローカル行列)
    DirectX::XMMATRIX nodeTransform = node->transformation;

    // もしこのノード（ボーン）のアニメーションデータが存在すれば、行列を上書きする
    if (currentClip->boneNameToTrackIndex.find(nodeName) != currentClip->boneNameToTrackIndex.end()) {
        int trackIndex = currentClip->boneNameToTrackIndex.at(nodeName);
        const BoneAnimationTrack& track = currentClip->boneTracks[trackIndex];

        // 補間計算
        DirectX::XMFLOAT3 pos = CalcInterpolatedPosition(currentTime, track);
        DirectX::XMFLOAT4 rot = CalcInterpolatedRotation(currentTime, track);
        DirectX::XMFLOAT3 scale = CalcInterpolatedScaling(currentTime, track);

        // スケール・回転・平行移動の行列を作成し、掛け合わせて新しいローカル行列を作る (S * R * T)
        DirectX::XMMATRIX matScale = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);
        DirectX::XMMATRIX matRot = DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&rot));
        DirectX::XMMATRIX matTrans = DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z);

        nodeTransform = matScale * matRot * matTrans;
    }

    // グローバル行列 = 自分のローカル行列 × 親のグローバル行列 (DirectXMathのRow-Major仕様)
    DirectX::XMMATRIX globalTransformation = nodeTransform * parentTransform;

    // もしこのノードが「ボーン」であれば、最終行列を計算して配列に保存
    if (boneInfoMap->find(nodeName) != boneInfoMap->end()) {
        const BoneInfo& boneInfo = boneInfoMap->at(nodeName);

        // 最終行列 = オフセット行列(初期姿勢に戻す行列) × 今のグローバル行列
        //finalBoneMatrices[boneInfo.id] = DirectX::XMMatrixTranspose(boneInfo.offsetMatrix * globalTransformation);
        finalBoneMatrices[boneInfo.id] = boneInfo.offsetMatrix * globalTransformation;
    }

    // 子ノードすべてに対して、自分のグローバル行列を「親行列」として渡して再帰呼び出し
    for (const NodeData& child : node->children) {
        CalculateBoneTransform(&child, globalTransformation);
    }
}

// 位置の補間 (線形補間: Lerp)
DirectX::XMFLOAT3 Animator::CalcInterpolatedPosition(float animationTime, const BoneAnimationTrack& track) {
    // キーフレームが1つしかなければそれをそのまま返す
    if (track.positions.size() == 1) return track.positions[0].position;

    // 現在の時間を含む2つのキーフレームを探す
    int p0Index = FindPositionIndex(animationTime, track);
    int p1Index = p0Index + 1;

    // 2つのキーフレーム間の時間の割合 (0.0f ～ 1.0f) を計算
    float t1 = track.positions[p0Index].timeStamp;
    float t2 = track.positions[p1Index].timeStamp;
    float deltaTime = t2 - t1;
    float factor = (animationTime - t1) / deltaTime;

    // DirectXMathを使ってLerp (線形補間)
    DirectX::XMVECTOR start = DirectX::XMLoadFloat3(&track.positions[p0Index].position);
    DirectX::XMVECTOR end = DirectX::XMLoadFloat3(&track.positions[p1Index].position);
    DirectX::XMVECTOR interp = DirectX::XMVectorLerp(start, end, factor);

    DirectX::XMFLOAT3 result;
    DirectX::XMStoreFloat3(&result, interp);
    return result;
}

// 回転の補間 (球面線形補間: Slerp)
DirectX::XMFLOAT4 Animator::CalcInterpolatedRotation(float animationTime, const BoneAnimationTrack& track) {
    if (track.rotations.size() == 1) return track.rotations[0].orientation;

    int p0Index = FindRotationIndex(animationTime, track);
    int p1Index = p0Index + 1;

    float t1 = track.rotations[p0Index].timeStamp;
    float t2 = track.rotations[p1Index].timeStamp;
    float deltaTime = t2 - t1;
    float factor = (animationTime - t1) / deltaTime;

    DirectX::XMVECTOR start = DirectX::XMLoadFloat4(&track.rotations[p0Index].orientation);
    DirectX::XMVECTOR end = DirectX::XMLoadFloat4(&track.rotations[p1Index].orientation);

    // Slerp
    DirectX::XMVECTOR interp = DirectX::XMQuaternionSlerp(start, end, factor);

    DirectX::XMFLOAT4 result;
    DirectX::XMStoreFloat4(&result, interp);
    return result;
}

// スケールの補間 (線形補間: Lerp)
DirectX::XMFLOAT3 Animator::CalcInterpolatedScaling(float animationTime, const BoneAnimationTrack& track) {
    if (track.scales.size() == 1) return track.scales[0].scale;

    int p0Index = FindScalingIndex(animationTime, track);
    int p1Index = p0Index + 1;

    float t1 = track.scales[p0Index].timeStamp;
    float t2 = track.scales[p1Index].timeStamp;
    float deltaTime = t2 - t1;
    float factor = (animationTime - t1) / deltaTime;

    DirectX::XMVECTOR start = DirectX::XMLoadFloat3(&track.scales[p0Index].scale);
    DirectX::XMVECTOR end = DirectX::XMLoadFloat3(&track.scales[p1Index].scale);
    DirectX::XMVECTOR interp = DirectX::XMVectorLerp(start, end, factor);

    DirectX::XMFLOAT3 result;
    DirectX::XMStoreFloat3(&result, interp);
    return result;
}