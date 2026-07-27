#include "DirectX12_pch.h"
#include "Model.h"
#include "GraphicsCore.h"
#include "Material.h"

void Model::CreateFromFile(const std::string& filePath) {

	ID3D12Device* device = GraphicsCore::Get().GetDevice();
        
    ID3D12GraphicsCommandList* cmdList = GraphicsCore::Get().GetCommandList();

    // CPU側でFBXを解析してデータを抽出 (さっき作ったローダーを使用)
    std::vector<MeshData> loadedData = ModelLoader::LoadFBX(filePath);

    // 抽出されたメッシュの数だけGPUバッファ(Meshクラス)を生成
    meshes.resize(loadedData.size());
    diffuseTextures.resize(loadedData.size());

    // FBXファイルがあるディレクトリのパスを抽出 (例: "Assets/Model.fbx" -> "Assets/")
    std::string directory = filePath.substr(0, filePath.find_last_of("/\\") + 1);

    for (size_t i = 0; i < loadedData.size(); ++i) {
        const auto& data = loadedData[i];

        // 抽出したポインタとサイズを、Mesh::Create にそのまま渡す
        meshes[i].Create(
            data.vertices.data(), static_cast<UINT>(data.vertices.size()), sizeof(VERTEX),
            data.indices.data(), static_cast<UINT>(data.indices.size()), DXGI_FORMAT_R32_UINT
        );

        // テクスチャパスがあればロード
        if (!data.material.DiffuseTexturePath.empty()) {
            std::string texPathUtf8 = directory + data.material.DiffuseTexturePath;

            // DirectXTex用に std::string -> std::wstring 変換
            int size_needed = MultiByteToWideChar(CP_UTF8, 0, &texPathUtf8[0], (int)texPathUtf8.size(), NULL, 0);
            std::wstring wTexPath(size_needed, 0);
            MultiByteToWideChar(CP_UTF8, 0, &texPathUtf8[0], (int)texPathUtf8.size(), &wTexPath[0], size_needed);

            // テクスチャの生成命令を積む
            diffuseTextures[i].CreateFromFile(wTexPath);
        }
    }

}
void Model::Draw(Material& material) {
	CommandContext& context = GraphicsCore::Get().GetCommandContext();
    // メッシュごとにテクスチャを切り替えて描画
    for (size_t i = 0; i < meshes.size(); ++i) {

        // テクスチャがあればマテリアルにセット
        // (Textureが正しく生成されてSRVを持っていればセットする)
        material.SetTexture(diffuseTextures[i].GetSrvGpuHandle());

        // MaterialのBind()を呼んで、コマンドリストにPSOや定数バッファ、DescriptorTableをセット
        material.Bind();

        // 頂点・インデックスバッファをセットして描画
        context.SetVertexBuffer(0, meshes[i].GetVertexBufferView());
        context.SetIndexBuffer(meshes[i].GetIndexBufferView());
        context.DrawIndexedInstanced(meshes[i].GetIndexCount(), 1, 0, 0, 0);
    }
}

void Model::FreeUploadBuffers() {
    // 保持しているすべてのメッシュの中間バッファを一括解放
    for (auto& mesh : meshes) {
        mesh.FreeUploadBuffers();
    }

    for (auto& tex : diffuseTextures) {
        tex.FreeUploadBuffer(); // テクスチャの中間バッファも解放
    }
}