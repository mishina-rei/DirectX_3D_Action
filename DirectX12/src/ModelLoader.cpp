#include "DirectX12_pch.h"

#include "ModelLoader.h"

void ModelLoader::SetVertexBoneData(VERTEX& vertex, int boneID, float weight) {
    for (int i = 0; i < MAX_BONE_INFLUENCE; ++i) {
        // まだ空いているスロット（IDが-1の場所）を見つけて格納
        if (vertex.BoneIDs[i] < 0) {
            vertex.BoneIDs[i] = boneID;
            vertex.BoneWeights[i] = weight;
            break;
        }
    }
}

DirectX::XMMATRIX ModelLoader::ConvertMatrixToDirectXFormat(const aiMatrix4x4& from) {
    DirectX::XMMATRIX to;
    // Assimpは行優先(Row-Major)、DirectXMathもデフォルトはRow-Major
    // メモリ配列が異なるため、各要素を直接マッピングして転置
    to = DirectX::XMMatrixSet(
        from.a1, from.b1, from.c1, from.d1,
        from.a2, from.b2, from.c2, from.d2,
        from.a3, from.b3, from.c3, from.d3,
        from.a4, from.b4, from.c4, from.d4
    );
    return to;
}

void ModelLoader::ExtractBoneWeights(std::vector<VERTEX>& vertices, aiMesh* mesh, MeshData& meshData) {
    for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
        int boneID = -1;
        std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();

        // 新しいボーンを発見した場合はMapに追加
        if (meshData.BoneInfoMap.find(boneName) == meshData.BoneInfoMap.end()) {
            BoneInfo newBoneInfo;
            newBoneInfo.id = meshData.BoneCounter;
            newBoneInfo.offsetMatrix = ConvertMatrixToDirectXFormat(mesh->mBones[boneIndex]->mOffsetMatrix);

            meshData.BoneInfoMap[boneName] = newBoneInfo;
            boneID = meshData.BoneCounter;
            meshData.BoneCounter++;
        }
        else {
            // 既に登録済みのボーンならIDを取得
            boneID = meshData.BoneInfoMap[boneName].id;
        }

        // このボーンが影響を与える頂点とその重み（ウェイト）を取得
        auto weights = mesh->mBones[boneIndex]->mWeights;
        int numWeights = mesh->mBones[boneIndex]->mNumWeights;

        for (int weightIndex = 0; weightIndex < numWeights; ++weightIndex) {
            int vertexId = weights[weightIndex].mVertexId;
            float weight = weights[weightIndex].mWeight;

            // Assimpは微小なウェイトを持つことがあるため、一定以下は無視する
            if (weight <= 0.0f) continue;

            // 該当する頂点にボーンIDとウェイトを設定
            SetVertexBoneData(vertices[vertexId], boneID, weight);
        }
    }
}

LoadedSceneData ModelLoader::LoadFBX(const std::string& filePath) {
    Assimp::Importer importer;

    // DirectX12向けに左手座標系に変換(ConvertToLeftHanded)し、
    // 多角形ポリゴンを三角形に分割(Triangulate)するフラグを渡す
    const aiScene* scene = importer.ReadFile(filePath,
        aiProcess_Triangulate |
        aiProcess_ConvertToLeftHanded |
        aiProcess_CalcTangentSpace);

    // シーン自体がない,ルートノードがないときにエラー出す
    if (!scene || !scene->mRootNode) {
        std::string errorStr = importer.GetErrorString();
        OutputDebugStringA(("Assimp Load Error: " + errorStr + "\n").c_str());
        throw std::runtime_error("Assimp Load Error: " + errorStr);
    }

    // メッシュがない（不完全）フラグが立っている場合の追加チェック
    if (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
        // アニメーションデータが入っているならアニメーション専用として許容する
        if (!scene->HasAnimations()) {
            std::string errorStr = importer.GetErrorString();
            OutputDebugStringA(("Assimp Load Error (Incomplete & No Animations): " + errorStr + "\n").c_str());
            throw std::runtime_error("Assimp Load Error (Incomplete & No Animations): " + errorStr);
        }
    }

    std::vector<MeshData> loadedMeshes;

    // シーン内のすべてのメッシュをループ
    for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
        aiMesh* ai_mesh = scene->mMeshes[i];
        MeshData meshData;

        // 頂点データの抽出
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

        // インデックスデータの抽出
        for (unsigned int f = 0; f < ai_mesh->mNumFaces; ++f) {
            aiFace face = ai_mesh->mFaces[f];
            for (unsigned int ind = 0; ind < face.mNumIndices; ++ind) {
                meshData.indices.push_back(face.mIndices[ind]);
            }
        }

        // マテリアル（テクスチャパス）の抽出
        if (ai_mesh->mMaterialIndex >= 0) {
            aiMaterial* material = scene->mMaterials[ai_mesh->mMaterialIndex];
            aiString texPath;
            // ディフューズ（アルベド）テクスチャのパスを取得
            if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == aiReturn_SUCCESS) {
                // C++の文字列に変換して保持
                meshData.material.DiffuseTexturePath = texPath.C_Str();
            }
        }

        ExtractBoneWeights(meshData.vertices, ai_mesh, meshData);

        loadedMeshes.push_back(meshData);
    }

    LoadedSceneData finalData;
    finalData.meshes = loadedMeshes; // これまで抽出したメッシュデータの配列
    finalData.animations = ExtractAnimations(scene); // ここでアニメーションを一気に抽出

    // RootNodeから階層構造を抜き出す
    if (scene->mRootNode) {
        finalData.rootNode = ExtractNodeHierarchy(scene->mRootNode);
    }
    return finalData;
}

DirectX::XMFLOAT3 ModelLoader::ConvertToXMFLOAT3(const aiVector3D& vec) {
    return DirectX::XMFLOAT3(vec.x, vec.y, vec.z);
}

DirectX::XMFLOAT4 ModelLoader::ConvertToXMFLOAT4(const aiQuaternion& pOrientation) {
    // クォータニオンの並び順 (x, y, z, w) をそのまま渡す
    return DirectX::XMFLOAT4(pOrientation.x, pOrientation.y, pOrientation.z, pOrientation.w);
}

std::vector<AnimationClip> ModelLoader::ExtractAnimations(const aiScene* scene) {
    std::vector<AnimationClip> animations;

    // アニメーションデータが含まれていなければ空の配列を返す
    if (!scene->HasAnimations()) {
        return animations;
    }

    // すべてのアニメーションクリップをループ
    for (unsigned int i = 0; i < scene->mNumAnimations; ++i) {
        aiAnimation* aiAnim = scene->mAnimations[i];
        AnimationClip clip;

        clip.name = aiAnim->mName.C_Str();
        clip.duration = static_cast<float>(aiAnim->mDuration);

        // TicksPerSecondが0の場合はデフォルト値（25.0fや30.0f）を設定
        clip.ticksPerSecond = static_cast<float>(aiAnim->mTicksPerSecond != 0 ? aiAnim->mTicksPerSecond : 30.0f);

        // このクリップに含まれるすべてのボーンの動き（チャンネル）をループ
        for (unsigned int j = 0; j < aiAnim->mNumChannels; ++j) {
            aiNodeAnim* channel = aiAnim->mChannels[j];
            BoneAnimationTrack track;
            track.boneName = channel->mNodeName.C_Str();

            // 位置キーフレームの抽出
            for (unsigned int posIndex = 0; posIndex < channel->mNumPositionKeys; ++posIndex) {
                aiVectorKey key = channel->mPositionKeys[posIndex];
                KeyPosition kp;
                kp.position = ConvertToXMFLOAT3(key.mValue);
                kp.timeStamp = static_cast<float>(key.mTime);
                track.positions.push_back(kp);
            }

            // 回転キーフレームの抽出 (クォータニオン)
            for (unsigned int rotIndex = 0; rotIndex < channel->mNumRotationKeys; ++rotIndex) {
                aiQuatKey key = channel->mRotationKeys[rotIndex];
                KeyRotation kr;
                kr.orientation = ConvertToXMFLOAT4(key.mValue);
                kr.timeStamp = static_cast<float>(key.mTime);
                track.rotations.push_back(kr);
            }

            // スケールキーフレームの抽出
            for (unsigned int scaleIndex = 0; scaleIndex < channel->mNumScalingKeys; ++scaleIndex) {
                aiVectorKey key = channel->mScalingKeys[scaleIndex];
                KeyScale ks;
                ks.scale = ConvertToXMFLOAT3(key.mValue);
                ks.timeStamp = static_cast<float>(key.mTime);
                track.scales.push_back(ks);
            }

            // 辞書への登録とトラックの追加
            clip.boneNameToTrackIndex[track.boneName] = static_cast<int>(clip.boneTracks.size());
            clip.boneTracks.push_back(track);
        }

        animations.push_back(clip);
    }

    return animations;
}

NodeData ModelLoader::ExtractNodeHierarchy(const aiNode* srcNode) {
    NodeData destNode;
    destNode.name = srcNode->mName.C_Str();

    // このノードのローカル初期姿勢をDirectXMathの行列に変換
    destNode.transformation = ConvertMatrixToDirectXFormat(srcNode->mTransformation);

    // 子ノードの数だけループして、自分自身（ExtractNodeHierarchy）を呼び出す
    destNode.children.reserve(srcNode->mNumChildren);
    for (unsigned int i = 0; i < srcNode->mNumChildren; i++) {
        destNode.children.push_back(ExtractNodeHierarchy(srcNode->mChildren[i]));
    }

    return destNode;
}