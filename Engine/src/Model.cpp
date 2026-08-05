#include "Engine_pch.h"
#include "Model.h"
#include <filesystem> 

void Model::CreateFromFile(const std::string& filePath) {

	ID3D12Device* device = GraphicsCore::Get().GetDevice();
        
    ID3D12GraphicsCommandList* cmdList = GraphicsCore::Get().GetCommandList();

    // CPU側でFBXを解析してデータを抽出
    auto loadedData = ModelLoader::LoadFBX(filePath);

    // 抽出されたメッシュの数だけGPUバッファ(Meshクラス)を生成
    meshes.resize(loadedData.meshes.size());
    diffuseTextures.resize(loadedData.meshes.size());

    // FBXファイルがあるディレクトリのパスを抽出 (例: "Assets/Model.fbx" -> "Assets/")
    std::string directory = filePath.substr(0, filePath.find_last_of("/\\") + 1);

    for (size_t i = 0; i < loadedData.meshes.size(); ++i) {
        const auto& data = loadedData.meshes[i];

        // 抽出したポインタとサイズを、Mesh::Create にそのまま渡す
        meshes[i].Create(
            data.vertices.data(), static_cast<UINT>(data.vertices.size()), sizeof(VERTEX),
            data.indices.data(), static_cast<UINT>(data.indices.size()), DXGI_FORMAT_R32_UINT
        );

        // テクスチャパスがあればロード
        if (!data.material.DiffuseTexturePath.empty()) {
			// パスからファイル名だけを抽出するために、std::string を使って処理
            std::string rawStr = data.material.DiffuseTexturePath;

            // パス区切り文字（\ または /）が最後に現れる位置を探す
            size_t pos = rawStr.find_last_of("/\\");

            // 区切り文字が見つかればそれ以降を、見つからなければそのまま文字列を使う
            std::string fileName = (pos == std::string::npos) ? rawStr : rawStr.substr(pos + 1);

            // FBXのディレクトリとファイル名を結合
            std::string texPathUtf8 = directory + fileName;

            // DirectXTex用に std::string -> std::wstring 変換
            int size_needed = MultiByteToWideChar(CP_UTF8, 0, &texPathUtf8[0], (int)texPathUtf8.size(), NULL, 0);
            std::wstring wTexPath(size_needed, 0);
            MultiByteToWideChar(CP_UTF8, 0, &texPathUtf8[0], (int)texPathUtf8.size(), &wTexPath[0], size_needed);

            // テクスチャの生成命令を積む
            diffuseTextures[i].CreateFromFile(wTexPath);
        }
    }

	// アニメーションとボーン情報を移動
	animations = std::move(loadedData.animations);
	boneInfoMap = std::move(loadedData.boneInfoMap);
	rootNode = std::move(loadedData.rootNode);
}
void Model::Draw(Material& material) {
	CommandContext& context = GraphicsCore::Get().GetCommandContext();

    if (currentAnimationIndex >= 0) {
        const auto& boneMatrices = animator.GetFinalBoneMatrices();
        if (!boneMatrices.empty()) {
            // 送信直前にのみTransposeをかける（暗黙のルールをここに隠蔽）
            //std::vector<DirectX::XMMATRIX> transposedBones(boneMatrices.size());
            //for (size_t i = 0; i < boneMatrices.size(); ++i) {
            //    transposedBones[i] = DirectX::XMMatrixTranspose(boneMatrices[i]);
            //}
            material.SetData("boneTransforms", boneMatrices.data(), sizeof(DirectX::XMMATRIX) * boneMatrices.size());
        }
    }
    else {
        // アニメーションがない、または再生されていない時のフェイルセーフ
        std::vector<DirectX::XMMATRIX> identityBones(256, DirectX::XMMatrixIdentity());
        material.SetData("boneTransforms", identityBones.data(), sizeof(DirectX::XMMATRIX) * 256);
    }

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

void Model::PlayAnimation(int index) {
    if (index >= 0 && index < animations.size()) {
        if (currentAnimationIndex != index) {
            currentAnimationIndex = index;
            // アニメーターの初期化
            animator.Initialize(&animations[index], &rootNode, &boneInfoMap);
        }
    }
}

void Model::Update(float dt) {
    if (currentAnimationIndex >= 0) {
        animator.UpdateAnimation(dt);
    }
}

void Model::AddAnimationClip(const AnimationClip& clip) {
    animations.push_back(clip);
}

void Model::CrossFadeAnimation(int index, float transitionDuration) {
    if (index >= 0 && index < animations.size()) {
        if (currentAnimationIndex != index) {
            currentAnimationIndex = index;
            // AnimatorのCrossFadeを呼ぶ
            animator.CrossFade(&animations[index], transitionDuration);
        }
    }
}