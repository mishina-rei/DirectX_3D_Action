// Archive.h
#pragma once
#include "imgui.h"
#include "json.hpp"
#include "Vector.h" 
#include "Quaternion.h"
#include <unordered_map>
#include "AssetRef.h"
#include "world.h"
#include "Name.h"
#include "UUID.h"


// 他のエンティティを参照するための専用の型
struct EntityRef {
    uint64_t uuid = 0;                         // セーブ/ロード用
    ECS::EntityID id = ECS::INVALID_ENTITY_ID; // ゲーム実行用
};

using json = nlohmann::json;

// ==========================================
// エディタ描画用アーカイバ (ImGui)
// ==========================================
class ImGuiArchive {
public:
    ECS::World* world = nullptr;
    ECS::EntityID currentEntityID = ECS::INVALID_ENTITY_ID;

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

	// Quaternion用 (オイラー角で表示)
    void Property(const char* name, Quaternion& value) {
        Vector3 euler = value.ToEuler();
        if (ImGui::DragFloat3(name, &euler.x, 0.5f)) {
            value = Quaternion::FromRotation(euler.x, euler.y, euler.z);
        }
    }

    // UUID用
    void Property(const char* name, uint64_t& value) {
        // 値はいじれないようにTextで表示するだけ
        ImGui::Text("%s: %llu", name, (unsigned long long)value);
    }
	// EntityRef用
    void Property(const char* name, EntityRef& value) {
        std::string currentParentName = "None";
        if (value.id != ECS::INVALID_ENTITY_ID && world && world->HasComponent<Name>(value.id)) {
            currentParentName = world->GetComponent<Name>(value.id).name;
        }

        ImGui::PushID(name);
        if (ImGui::BeginCombo(name, currentParentName.c_str())) {
            // "None"（親なし）の選択肢
            bool isNoneSelected = (value.id == ECS::INVALID_ENTITY_ID);
            if (ImGui::Selectable("None", isNoneSelected)) {
                value.id = ECS::INVALID_ENTITY_ID;
                value.uuid = 0;
            }

            // シーン内の他のエンティティを一覧表示
            if (world) {
                world->ForEachComponent<Name>([&](ECS::EntityID entityId, Name& entityName) {
                    if (entityId == currentEntityID) return; // 自分自身は除外

                    bool isSelected = (value.id == entityId);
                    std::string label = entityName.name + " (ID: " + std::to_string(entityId) + ")";
                    if (ImGui::Selectable(label.c_str(), isSelected)) {
                        value.id = entityId;
                        if (world->HasComponent<UUIDComponent>(entityId)) {
                            value.uuid = world->GetComponent<UUIDComponent>(entityId).id;
                        }
                    }
                });
            }
            ImGui::EndCombo();
        }
        ImGui::PopID();
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
    // ロード時に作ったUUID -> 新EntityIDの対応表
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