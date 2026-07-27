#pragma once
#include <d3d12.h>
#include <string>
#include <vector>
#include "Mesh.h"
#include "ModelLoader.h"
#include "Texture.h"

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

    std::vector<Mesh> meshes;               // FBX内のすべてのメッシュを保持
    std::vector<Texture> diffuseTextures;   // メッシュごとのディフューズテクスチャ
};