// Archive.h
#pragma once
#include "imgui.h"
#include "json.hpp"
#include "Vector.h" 
#include "Quaternion.h"
#include <unordered_map>

// 他のエンティティを参照するための専用の型
struct EntityRef {
    uint64_t uuid = 0;                         // セーブ/ロード用
    ECS::EntityID id = ECS::INVALID_ENTITY_ID; // ゲーム実行用
};

// ==========================================
// アセット読み込み用のヘルパー関数
// ==========================================
class Model;
class Texture;

inline void LoadAssetResource(std::shared_ptr<Model>& asset, const std::string& path);
inline void LoadAssetResource(std::shared_ptr<Texture>& asset, const std::string& path);

// アセットのファイルパスと読み込み済み実体を管理する型
template<typename T>
struct AssetRef {
    std::string path = "";
    std::shared_ptr<T> asset = nullptr;

    void Load(const std::string& newPath) {
        path = newPath;
        LoadAssetResource(asset, path);
    }

    T* operator->() const { return asset.get(); }
    T* get() const { return asset.get(); }
    operator bool() const { return asset != nullptr; }
};

using json = nlohmann::json;

// ==========================================
// エディタ描画用アーカイバ (ImGui)
// ==========================================
class ImGuiArchive {
public:
    void Property(const char* name, float& value) { ImGui::DragFloat(name, &value, 0.1f); }
    void Property(const char* name, int& value) { ImGui::DragInt(name, &value); }
    void Property(const char* name, bool& value) { ImGui::Checkbox(name, &value); }
    void Property(const char* name, Vector2& value) { ImGui::DragFloat2(name, &value.x, 0.1f); }
    void Property(const char* name, Vector3& value) { ImGui::DragFloat3(name, &value.x, 0.1f); }
    void Property(const char* name, Vector4& value) { ImGui::DragFloat4(name, &value.x, 0.1f); }
    void Property(const char* name, std::string& value) {
        char buf[256];
        strcpy_s(buf, value.c_str());
        if (ImGui::InputText(name, buf, sizeof(buf))) {
            value = buf;
        }
    }
    void Property(const char* name, Quaternion& value) {
        ImGui::DragFloat4(name, &value.x, 0.01f);
    }

    // UUID用
    void Property(const char* name, uint64_t& value) {
        // 値はいじれないようにTextで表示するだけ
        ImGui::Text("%s: %llu", name, (unsigned long long)value);
    }
	// EntityRef用
    void Property(const char* name, EntityRef& value) {
        // UIテキスト表示
        ImGui::Text("%s [Target ID: %d]", name, (int)value.id);
    }
    // AssetRef用
    template<typename T>
    void Property(const char* name, AssetRef<T>& value) {
        char buf[256];
        strcpy_s(buf, value.path.c_str());
        ImGui::PushID(name);
        if (ImGui::InputText(name, buf, sizeof(buf))) {
            value.path = buf;
        }
        ImGui::SameLine();
        if (ImGui::Button("Reload")) {
            value.Load(value.path);
        }
        ImGui::PopID();
    }
};

// ==========================================
// セーブデータ書き込み用アーカイバ (JSON Write)
// ==========================================
class JsonWriteArchive {
public:
    json j; // 書き込み先のJSONオブジェクト

    void Property(const char* name, float& value) { j[name] = value; }
    void Property(const char* name, int& value) { j[name] = value; }
    void Property(const char* name, bool& value) { j[name] = value; }
    void Property(const char* name, Vector2& value) { j[name] = { value.x, value.y }; }
    void Property(const char* name, Vector3& value) { j[name] = { value.x, value.y, value.z }; }
    void Property(const char* name, Vector4& value) { j[name] = { value.x, value.y, value.z, value.w }; }
    void Property(const char* name, std::string& value) { j[name] = value; }
    void Property(const char* name, Quaternion& value) {
        j[name] = { value.x, value.y, value.z, value.w };
    }
	// UUID用
    void Property(const char* name, uint64_t& value) { j[name] = value; }
	// EntityRef用
    void Property(const char* name, EntityRef& value) {
        j[name] = value.uuid; // 保存するのは絶対変わらないUUIDのみ
    }
    // AssetRef用
    template<typename T>
    void Property(const char* name, AssetRef<T>& value) {
        j[name] = value.path;
    }
};

// ==========================================
// セーブデータ読み込み用アーカイバ (JSON Read)
// ==========================================
class JsonReadArchive {
private:
    const json& j;
public:
    JsonReadArchive(const json& parsedJson) : j(parsedJson) {}

    void Property(const char* name, float& value) { if (j.contains(name)) value = j[name].get<float>(); }
    void Property(const char* name, int& value) { if (j.contains(name)) value = j[name].get<int>(); }
    void Property(const char* name, bool& value) { if (j.contains(name)) value = j[name].get<bool>(); }
    void Property(const char* name, Vector2& value) {
        if (j.contains(name) && j[name].is_array() && j[name].size() >= 2) {
            value.x = j[name][0]; value.y = j[name][1];
        }
    }
    void Property(const char* name, Vector3& value) {
        if (j.contains(name) && j[name].is_array() && j[name].size() >= 3) {
            value.x = j[name][0]; value.y = j[name][1]; value.z = j[name][2];
        }
    }
    void Property(const char* name, Vector4& value) {
        if (j.contains(name) && j[name].is_array() && j[name].size() >= 4) {
            value.x = j[name][0]; value.y = j[name][1]; value.z = j[name][2]; value.w = j[name][3];
        }
    }
    void Property(const char* name, std::string& value) {
        if (j.contains(name)) value = j[name].get<std::string>();
    }
    void Property(const char* name, Quaternion& value) {
        if (j.contains(name) && j[name].is_array() && j[name].size() >= 4) {
            value.x = j[name][0]; value.y = j[name][1]; value.z = j[name][2]; value.w = j[name][3];
        }
    }
	// UUID用
    void Property(const char* name, uint64_t& value) {
        if (j.contains(name)) value = j[name].get<uint64_t>();
    }
	// EntityRef用
    void Property(const char* name, EntityRef& value) {
        if (j.contains(name)) {
            value.uuid = j[name].get<uint64_t>();
            value.id = ECS::INVALID_ENTITY_ID; // 読み込み直後は新しいIDが不明なので無効化
        }
    }
    // AssetRef用
    template<typename T>
    void Property(const char* name, AssetRef<T>& value) {
        if (j.contains(name)) {
            std::string p = j[name].get<std::string>();
            value.Load(p);
        }
    }
};

// ==========================================
// リンク解決専用アーカイバ
// ==========================================
class ResolveArchive {
public:
    // ロード時に作った「UUID -> 新EntityID」の対応表
    const std::unordered_map<uint64_t, ECS::EntityID>& uuidMap;

    ResolveArchive(const std::unordered_map<uint64_t, ECS::EntityID>& map) : uuidMap(map) {}

    // EntityRef が来た時だけ、辞書から最新のEntityIDを見つけて上書きする
    void Property(const char* name, EntityRef& value) {
        if (value.uuid != 0 && uuidMap.find(value.uuid) != uuidMap.end()) {
            value.id = uuidMap.at(value.uuid);
        }
        else {
            value.id = ECS::INVALID_ENTITY_ID;
        }
    }

    // FloatやVector3など、EntityRef以外の型が来たらすべて無視する
    template<typename T>
    void Property(const char* name, T& value) {}
};

// ==========================================
// LoadAssetResource の実体定義
// ==========================================
#include "Model.h"
#include "Texture.h"

inline void LoadAssetResource(std::shared_ptr<Model>& asset, const std::string& path) {
    if (path.empty()) {
        asset = nullptr;
        return;
    }
    if (!asset) {
        asset = std::make_shared<Model>();
    }
    asset->CreateFromFile(path);
}

inline void LoadAssetResource(std::shared_ptr<Texture>& asset, const std::string& path) {
    if (path.empty()) {
        asset = nullptr;
        return;
    }
    if (!asset) {
        asset = std::make_shared<Texture>();
    }
    int size = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), (int)path.size(), NULL, 0);
    std::wstring wpath(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, path.c_str(), (int)path.size(), &wpath[0], size);
    asset->CreateFromFile(wpath);
}