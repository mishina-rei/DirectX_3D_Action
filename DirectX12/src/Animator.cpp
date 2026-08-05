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

    // 現在のアニメーションの時間を進める
    currentTime += dt * currentClip->ticksPerSecond;
    currentTime = fmod(currentTime, currentClip->duration);

    float blendFactor = 0.0f; // 0.0(現在) ～ 1.0(次)

    if (isBlending && nextClip) {
        // 次のアニメーションの時間も進める
        nextTime += dt * nextClip->ticksPerSecond;
        nextTime = fmod(nextTime, nextClip->duration);

        // ブレンドの進行度を更新
        currentBlendTime += dt;
        blendFactor = currentBlendTime / blendDuration;

        // ブレンド完了判定
        if (blendFactor >= 1.0f) {
            currentClip = nextClip;
            currentTime = nextTime;

            nextClip = nullptr;
            isBlending = false;
            blendFactor = 0.0f;
        }
    }

    // 階層を辿って行列を計算する
    CalculateBoneTransform(rootNode, DirectX::XMMatrixIdentity(), blendFactor);
}

void Animator::CalculateBoneTransform(const NodeData* node, DirectX::XMMATRIX parentTransform, float blendFactor) {
    std::string nodeName = node->name;

    // 基本はノードが元々持っている初期姿勢(ローカル行列)
    DirectX::XMMATRIX nodeTransform = node->transformation;

    // 現在のアニメーション（currentClip）のローカル姿勢を計算
    DirectX::XMVECTOR pos1, scale1, rot1;
    bool hasCurrentAnim = GetLocalTransform(currentClip, nodeName, currentTime, pos1, scale1, rot1);

    if (hasCurrentAnim) {
        // ブレンド中であれば、次のアニメーションの姿勢も計算してミックスする
        if (isBlending && nextClip) {
            DirectX::XMVECTOR pos2, scale2, rot2;
            bool hasNextAnim = GetLocalTransform(nextClip, nodeName, nextTime, pos2, scale2, rot2);

            if (hasNextAnim) {
                // ▼ ここがブレンドの魔法！ DirectXMathで補間する ▼

                // 位置の線形補間 (Lerp)
                pos1 = DirectX::XMVectorLerp(pos1, pos2, blendFactor);

                // スケールの線形補間 (Lerp)
                scale1 = DirectX::XMVectorLerp(scale1, scale2, blendFactor);

                // 回転の球面線形補間 (Slerp)
                rot1 = DirectX::XMQuaternionSlerp(rot1, rot2, blendFactor);
            }
        }

        // 抽出・補間した成分を行列に合成 (Scale * Rotation * Translation)
        DirectX::XMMATRIX matScale = DirectX::XMMatrixScalingFromVector(scale1);
        DirectX::XMMATRIX matRot = DirectX::XMMatrixRotationQuaternion(rot1);
        DirectX::XMMATRIX matTrans = DirectX::XMMatrixTranslationFromVector(pos1);

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
        CalculateBoneTransform(&child, globalTransformation, blendFactor);
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

void Animator::CrossFade(const AnimationClip* targetClip, float transitionDuration) {
    // 既に同じアニメーションがセットされている、または目標が空なら何もしない
    if (!targetClip || currentClip == targetClip) return;

    // もし現在何も再生していなければ、ブレンドせずに即座に切り替え
    if (!currentClip) {
        currentClip = targetClip;
        currentTime = 0.0f;
        isBlending = false;
        return;
    }

    // ブレンドの準備
    nextClip = targetClip;
    nextTime = 0.0f; // 次のアニメーションは最初から再生する

    blendDuration = transitionDuration;
    currentBlendTime = 0.0f;
    isBlending = true;
}

bool Animator::GetLocalTransform(const AnimationClip* clip, const std::string& nodeName, float time,
    DirectX::XMVECTOR& outPos, DirectX::XMVECTOR& outScale, DirectX::XMVECTOR& outRot)
{
    // このボーンのアニメーショントラックが存在するか辞書でチェック
    auto it = clip->boneNameToTrackIndex.find(nodeName);
    if (it == clip->boneNameToTrackIndex.end()) {
        return false; // アニメーションが無い（静止しているボーンなど）
    }

    // トラックを取得
    const BoneAnimationTrack& track = clip->boneTracks[it->second];

    // 各成分を補間計算して取得

	auto pos = CalcInterpolatedPosition(time, track);
	auto rot = CalcInterpolatedRotation(time, track);
	auto scale = CalcInterpolatedScaling(time, track);
    
    outPos = DirectX::XMLoadFloat3(&pos);
    outRot = DirectX::XMLoadFloat4(&rot);
    outScale = DirectX::XMLoadFloat3(&scale);

    return true;
}