#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <stdexcept>
#include <DirectXMath.h>
#include <vector>
#include <string>


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

// 1頂点のデータ
struct VERTEX {
    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT3 Normal;
    DirectX::XMFLOAT2 TexCoord;
};

// マテリアル情報（とりあえず今回はディフューズテクスチャのパスだけ）
struct MaterialData {
    std::string DiffuseTexturePath;
};

// 1つのメッシュ（FBXの中に複数のメッシュが含まれることが多い）
struct MeshData {
    std::vector<VERTEX> vertices;
    std::vector<uint32_t> indices;
    MaterialData material;
};

class ModelLoader {
public:
    static std::vector<MeshData> LoadFBX(const std::string& filePath) {
        Assimp::Importer importer;

        // 【超重要】DirectX12向けに左手座標系に変換(ConvertToLeftHanded)し、
        // 多角形ポリゴンを三角形に分割(Triangulate)するフラグを渡す
        const aiScene* scene = importer.ReadFile(filePath,
            aiProcess_Triangulate |
            aiProcess_ConvertToLeftHanded |
            aiProcess_CalcTangentSpace);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            // エラー内容を変数に受けて、出力ウィンドウに表示する
            std::string errorStr = importer.GetErrorString();
            OutputDebugStringA(("Assimp Load Error: " + errorStr + "\n").c_str());

            throw std::runtime_error("Assimp Load Error: " + errorStr);
        }

        std::vector<MeshData> loadedMeshes;

        // シーン内のすべてのメッシュをループ
        for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
            aiMesh* ai_mesh = scene->mMeshes[i];
            MeshData meshData;

            // ① 頂点データの抽出
            for (unsigned int v = 0; v < ai_mesh->mNumVertices; ++v) {
                VERTEX vertex;
                // 位置
                vertex.Position = { ai_mesh->mVertices[v].x, ai_mesh->mVertices[v].y, ai_mesh->mVertices[v].z };

                // 法線
                if (ai_mesh->HasNormals()) {
                    vertex.Normal = { ai_mesh->mNormals[v].x, ai_mesh->mNormals[v].y, ai_mesh->mNormals[v].z };
                }
                else {
                    vertex.Normal = { 0.0f, 0.0f, 0.0f };
                }

                // UV座標 (テクスチャチャネル0番)
                if (ai_mesh->mTextureCoords[0]) {
                    vertex.TexCoord = { ai_mesh->mTextureCoords[0][v].x, ai_mesh->mTextureCoords[0][v].y };
                }
                else {
                    vertex.TexCoord = { 0.0f, 0.0f };
                }

                meshData.vertices.push_back(vertex);
            }

            // ② インデックスデータの抽出
            for (unsigned int f = 0; f < ai_mesh->mNumFaces; ++f) {
                aiFace face = ai_mesh->mFaces[f];
                for (unsigned int ind = 0; ind < face.mNumIndices; ++ind) {
                    meshData.indices.push_back(face.mIndices[ind]);
                }
            }

            // ③ マテリアル（テクスチャパス）の抽出
            if (ai_mesh->mMaterialIndex >= 0) {
                aiMaterial* material = scene->mMaterials[ai_mesh->mMaterialIndex];
                aiString texPath;
                // ディフューズ（アルベド）テクスチャのパスを取得
                if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == aiReturn_SUCCESS) {
                    // C++の文字列に変換して保持
                    meshData.material.DiffuseTexturePath = texPath.C_Str();
                }
            }

            loadedMeshes.push_back(meshData);
        }

        return loadedMeshes;
    }
};