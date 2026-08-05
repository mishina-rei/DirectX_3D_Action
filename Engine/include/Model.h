#pragma once
#include <d3d12.h>
#include <string>
#include <vector>
#include "Mesh.h"
#include "ModelLoader.h"
#include "Texture.h"
#include "Animator.h"

class Material;

class Model {
public:
    Model() = default;
    ~Model() = default;

    // ファイルパスを受け取り、内部でデータ抽出とVRAM転送命令を積む
    void CreateFromFile(const std::string& filePath);

    // 転送完了後に中間バッファを一括で解放する
    void FreeUploadBuffers();

    // 将来的にはここに追加していく
    void Draw(Material& material);

    // アニメーション再生（インデックス指定）
    void PlayAnimation(int index);

    void Update(float dt);

    void AddAnimationClip(const AnimationClip& clip);

    // index番目のアニメーションへ滑らかに遷移する
    void CrossFadeAnimation(int index, float transitionDuration = 0.2f);

private:

    std::vector<Mesh> meshes;               // FBX内のすべてのメッシュを保持
    std::vector<Texture> diffuseTextures;   // メッシュごとのディフューズテクスチャ
    std::unordered_map<std::string, BoneInfo> boneInfoMap;
    NodeData rootNode;
    std::vector<AnimationClip> animations;

    int currentAnimationIndex = -1;
    Animator animator;
};