#pragma once
//#include <imgui/imgui.h>
//#include <imgui/imgui_impl_win32.h>
//#include <imgui/imgui_impl_dx12.h>
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx12.h>
#include <string>
#include <vector>
#include "world.h"
#include "ComponentMetaData.h"
#include "Archive.h"
#include "DescriptorHeapManager.h"

class EditorUI {
public:

	static EditorUI& Get() {
		static EditorUI instance;
		return instance;
	}

    void Initialize(HWND hwnd);
    void Shutdown();

    void BeginUI();
    void RenderUI(ECS::World* world);
    void DrawImGui();
    void Update(ECS::World* world);

    bool IsPlaying() const
    {
        return editorState == EditorState::Playing;
    }

    void SaveScene(ECS::World* world, const std::string& filepath);
    void LoadScene(ECS::World* world, const std::string& filepath);

    // コンポーネント登録用の関数
    template<typename T>
    void RegisterComponent(const std::string& name)
    {
        ComponentMetaData info;
        info.name = name;

        // インスペクター描画
        info.drawInspector = [name](ECS::World* world, ECS::EntityID id) {
            if (world->HasComponent<T>(id)) {
                if (ImGui::CollapsingHeader(name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                    auto& comp = world->GetComponent<T>(id);
                    ImGuiArchive archive;
                    archive.world = world;
                    archive.currentEntityID = id;
                    comp.Reflect(archive); // UI描画アーカイバを渡す
                }
            }
            };

        // コンポーネント追加
        info.addComponent = [name](ECS::World* world, ECS::EntityID id) {
            if (!world->HasComponent<T>(id)) {
                if (ImGui::Selectable(name.c_str())) {
                    world->AddComponent<T>(id, T{});
                }
            }
            };

        // JSONへ保存
        info.serialize = [name](ECS::World* world, ECS::EntityID id, json& outJson) {
            if (world->HasComponent<T>(id)) {
                auto& comp = world->GetComponent<T>(id);
                JsonWriteArchive archive;
                comp.Reflect(archive);
                outJson["Components"][name] = archive.j;
            }
            };

        // JSONから復元
        info.deserialize = [name](ECS::World* world, ECS::EntityID id, const json& inJson) {
            if (inJson.contains("Components") && inJson["Components"].contains(name)) {
                T comp{};
                if (world->HasComponent<T>(id)) {
                    comp = world->GetComponent<T>(id);
                }
                JsonReadArchive archive(inJson["Components"][name]);
                comp.Reflect(archive);
                world->AddComponent<T>(id, comp);
            }
            };

        // リンクの解決
        info.resolveLinks = [name](ECS::World* world, ECS::EntityID id, const std::unordered_map<uint64_t, ECS::EntityID>& uuidMap) {
            if (world->HasComponent<T>(id)) {
                auto& comp = world->GetComponent<T>(id);
                // リンク解決専用アーカイバ
                ResolveArchive archive(uuidMap);
                comp.Reflect(archive);
            }
            };

        componentRegistry.push_back(info);
    }

private:
    enum class EditorState
    {
        Edit,
        Playing,
        Paused
    };

    EditorState editorState = EditorState::Edit;

	enum class EditorAction // 再生・停止の予約アクションを表す列挙型
    {
        None,
        Play,
        Stop
    };

	EditorAction pendingAction = EditorAction::None;    // 再生・停止の予約アクション

	json editSceneSnapshot;     // シーン再生前の状態をメモリに保存するためのJSONオブジェクト
	float editTimeScale = 1.0f; // シーン再生前の時間のスケールを保存するための変数
	std::string playbackError;  // 再生中のエラーを保持する文字列
	
    // 登録されたコンポーネントメタデータのリスト
    std::vector<ComponentMetaData> componentRegistry;

    // 現在選択中のエンティティID
    ECS::EntityID selectedEntityID = ECS::INVALID_ENTITY_ID;

	// 削除予定のエンティティID
    ECS::EntityID pendingDeleteEntity = ECS::INVALID_ENTITY_ID;

    // ディスクリプタ
    DescriptorHandle handle;

    // ロード予約用のフラグ
    std::string nextScene = "";

    void StartPlay(ECS::World* world);      // シーン再生の開始
    void StopPlay(ECS::World* world);       // シーン再生の停止

    json CaptureScene(ECS::World* world) const;         // シーンをJSONに保存
    bool RestoreScene(ECS::World* world, const json& sceneJson);    // JSONからシーンを復元

    void DrawHierarchyWindow(ECS::World* world);
	void DeleteEditorEntity(ECS::World* world, ECS::EntityID id);   // エディタ上でエンティティを削除する
    void DrawInspectorWindow(ECS::World* world);

};
