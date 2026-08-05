#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <stdexcept>
#include <DirectXMath.h>
#include <vector>
#include <string>
#include <unordered_map>


#if _MSC_VER >= 1930
#ifdef _DEBUG
#pragma comment(lib, "assimp-vc143-mtd.lib")
#else
#pragma comment(lib, "assimp-vc143-mt.lib")
#endif
#elif _MSC_VER >= 1920
#ifdef _DEBUG
#pragma comment(lib, "assimp-vc142-mtd.lib")
#else
#pragma comment(lib, "assimp-vc142-mt.lib")
#endif
#elif _MSC_VER >= 1910
#ifdef _DEBUG
#pragma comment(lib, "assimp-vc141-mtd.lib")
#else
#pragma comment(lib, "assimp-vc141-mt.lib")
#endif
#endif

constexpr int MAX_BONE_INFLUENCE = 4;  // 1頂点に影響するボーンの最大数

// 1頂点のデータ
struct VERTEX {
    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT3 Normal;
    DirectX::XMFLOAT2 TexCoord;
    // アニメーション用の追加データ
    int BoneIDs[MAX_BONE_INFLUENCE];
    float BoneWeights[MAX_BONE_INFLUENCE];

    VERTEX() {
        // シェーダーで配列外参照してクラッシュするのを防ぐ
        for (int i = 0; i < MAX_BONE_INFLUENCE; i++) {
            BoneIDs[i] = -1;       // -1 は影響ボーンなし
            BoneWeights[i] = 0.0f;
        }
    }
};

// マテリアル情報（とりあえず今回はディフューズテクスチャのパスだけ）
struct MaterialData {
    std::string DiffuseTexturePath;
};

struct BoneInfo {
    int id; // 最終的にシェーダーに送る行列配列のインデックス
    DirectX::XMMATRIX offsetMatrix; // ボーン空間へ変換するための行列（Inverse Bind Matrix）
};

// 1つのメッシュ
struct MeshData {
    std::vector<VERTEX> vertices;
    std::vector<uint32_t> indices;
    MaterialData material;
};

// -------------------------------------------------
// キーフレーム構造体
// -------------------------------------------------
struct KeyPosition {
    DirectX::XMFLOAT3 position;
    float timeStamp;
};

struct KeyRotation {
    DirectX::XMFLOAT4 orientation; // クォータニオン(x, y, z, w)
    float timeStamp;
};

struct KeyScale {
    DirectX::XMFLOAT3 scale;
    float timeStamp;
};

// -------------------------------------------------
// 1つのボーン（ノード）のアニメーション軌跡
// -------------------------------------------------
struct BoneAnimationTrack {
    std::string boneName;
    std::vector<KeyPosition> positions;
    std::vector<KeyRotation> rotations;
    std::vector<KeyScale> scales;
};

// -------------------------------------------------
// アニメーションクリップ全体（例：「Walk」「Run」など）
// -------------------------------------------------
struct AnimationClip {
    std::string name;
    float duration;       // アニメーションの総時間（ティック数）
    float ticksPerSecond; // 1秒あたりのティック数（再生速度の基準）

    std::vector<BoneAnimationTrack> boneTracks;

    // アニメーション計算時にボーン名から高速にトラックを探せるようにする辞書
    std::unordered_map<std::string, int> boneNameToTrackIndex;
};

// ボーンの親子関係（階層構造）を保持するノード
struct NodeData {
    std::string name;
    DirectX::XMMATRIX transformation; // そのノードの初期ローカル行列
    std::vector<NodeData> children;   // 子ノードの配列
};

// 読み込んだシーン全体のデータ
struct LoadedSceneData {
    std::vector<MeshData> meshes;
    std::vector<AnimationClip> animations;
    NodeData rootNode;
    std::unordered_map<std::string, BoneInfo> boneInfoMap;
};


class ModelLoader {
public:

    // 頂点にボーンIDとウェイトを追加するヘルパー関数
    static void SetVertexBoneData(VERTEX& vertex, int boneID, float weight);

    // Assimpの行列をDirectXMathの行列に変換するヘルパー関数
    static DirectX::XMMATRIX ConvertMatrixToDirectXFormat(const aiMatrix4x4& from);

    // LoadFBX内のメッシュ抽出処理の追加部分
    static void ExtractBoneWeights(std::vector<VERTEX>& vertices, aiMesh* mesh, std::unordered_map<std::string, BoneInfo>& globalBoneInfoMap,
        int& globalBoneCounter);
    
    static LoadedSceneData LoadFBX(const std::string& filePath);

    // AssimpのVector3DをDirectXMathのXMFLOAT3に変換
    static DirectX::XMFLOAT3 ConvertToXMFLOAT3(const aiVector3D& vec);

    // AssimpのQuaternionをDirectXMathのXMFLOAT4に変換
    static DirectX::XMFLOAT4 ConvertToXMFLOAT4(const aiQuaternion& pOrientation);

    static std::vector<AnimationClip> ExtractAnimations(const aiScene* scene);

    // aiNodeの階層を再帰的に自作のNodeDataに変換する関数
    static NodeData ExtractNodeHierarchy(const aiNode* srcNode);
};