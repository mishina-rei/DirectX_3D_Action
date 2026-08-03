#pragma once
#include <DirectXMath.h>
#include <vector>
#include <unordered_map>
#include "ModelLoader.h" // LoadedSceneData などの定義があるヘッダー

class Animator {
public:
    Animator() = default;

    // アニメーションの初期化（再生するクリップやモデルの階層データを渡す）
    void Initialize(const AnimationClip* clip, const NodeData* rootNode, const std::unordered_map<std::string, BoneInfo>* boneInfoMap);

    // 毎フレーム呼ぶ更新処理 (dt = 経過時間)
    void UpdateAnimation(float dt);

    // シェーダーに送るための最終的なボーン行列配列を取得
    const std::vector<DirectX::XMMATRIX>& GetFinalBoneMatrices() const { return finalBoneMatrices; }

private:
    // 再生中のデータへのポインタ（参照）
    const AnimationClip* currentClip = nullptr;
    const NodeData* rootNode = nullptr;
    const std::unordered_map<std::string, BoneInfo>* boneInfoMap = nullptr;

    float currentTime = 0.0f; // 現在のアニメーション再生時間
    std::vector<DirectX::XMMATRIX> finalBoneMatrices; // GPUに送る行列配列（最大ボーン数分）

    // 階層をたどって行列を計算する再帰関数
    void CalculateBoneTransform(const NodeData* node, DirectX::XMMATRIX parentTransform);

    // --- キーフレーム補間用のヘルパー関数 ---
    DirectX::XMFLOAT3 CalcInterpolatedPosition(float animationTime, const BoneAnimationTrack& track);
    DirectX::XMFLOAT4 CalcInterpolatedRotation(float animationTime, const BoneAnimationTrack& track);
    DirectX::XMFLOAT3 CalcInterpolatedScaling(float animationTime, const BoneAnimationTrack& track);
};