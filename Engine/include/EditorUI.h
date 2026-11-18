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
#include "SceneSerializer.h"
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
    void LoadStartupScene(ECS::World* world); // 最後に保存したシーンを起動時に開く

    // コンポーネント登録用の関数
    template<typename T>
    void RegisterComponent(const std::string& name, bool canRemove = true)
    {
        ComponentMetaData info;
        info.name = name;
        info.componentTypeId = ECS::ComponentRegistry::GetID<T>();
        info.canRemove = canRemove;
        info.hasComponent = [](ECS::World* world, ECS::EntityID id) {
			return world->HasComponent<T>(id);  // コンポーネントが存在するかどうかを判定
        };

		// コンポーネント削除用の関数を登録
        info.removeComponent = [](ECS::World* world, ECS::EntityID id) {
            if (world->HasComponent<T>(id))
            {
				world->RemoveComponent<T>(id);  // コンポーネントを削除
            }
        };

        // インスペクター描画
        info.drawInspector = [this, name, canRemove](ECS::World* world, ECS::EntityID id) {
            if (world->HasComponent<T>(id)) {
                if (ImGui::CollapsingHeader(name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                    if (canRemove)
                    {
                        ImGui::PushID(name.c_str());
						ImGui::BeginDisabled(editorState != EditorState::Edit); // 再生中は削除ボタンを無効化
                        if (ImGui::Button("Remove"))    // コンポーネント削除ボタン
                        {
                            pendingRemoveEntity = id;   // 削除するコンポーネントが付けられているエンティティを記録
                            pendingRemoveComponentTypeId = ECS::ComponentRegistry::GetID<T>();  // コンポーネントも記録
                        }
                        ImGui::EndDisabled();
                        ImGui::PopID();
                    }

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

        // 保存・復元の登録はゲームと共有する初期化処理で行う

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
    uint64_t observedSceneRevision = 0;

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

    // コンポーネント削除は次フレームの描画前に実行する
    ECS::EntityID pendingRemoveEntity = ECS::INVALID_ENTITY_ID;
    ECS::ComponentTypeID pendingRemoveComponentTypeId = 0;
    void RemoveEditorComponent(ECS::World* world, ECS::EntityID id, ECS::ComponentTypeID componentTypeId);

    // ディスクリプタ
    DescriptorHandle handle;

	enum class SceneOperation {     // シーン操作の種類
		None,   // 何もしない
		New,    // 新規作成
		Open,   // 開く
		Save    // 保存
    };

    enum class SceneDialog { 
		None,           // ダイアログなし
		ConfirmSwitch,  // シーン切り替え確認
		Open,           // シーンを開く
		SaveAs,         // シーンを名前を付けて保存
		Overwrite       // 上書き保存
    };

	std::string currentScenePath;       // 現在編集中のシーンのパス
	std::string sceneError;             // シーン操作中のエラーを保持する文字列
	std::vector<std::string> sceneFiles;    // シーンファイルのリスト

	char sceneName[128] = {};           // シーン名入力用のバッファ

	SceneDialog sceneDialog = SceneDialog::None;                    // 現在表示中のシーンダイアログの種類
	SceneOperation pendingSceneOperation = SceneOperation::None;    // 次フレームで実行するシーン操作の種類
	SceneOperation requestedSceneOperation = SceneOperation::None;  // ユーザーが要求したシーン操作の種類

	std::string pendingScenePath;       // 次フレームで実行するシーン操作のパス
	std::string requestedScenePath;     // ユーザーが要求したシーン操作のパス

	void NewScene(ECS::World* world);                                   // 新規シーンを作成する
	bool OpenScene(ECS::World* world, const std::string& filepath);     // シーンを開く
	bool SaveSceneFile(ECS::World* world, const std::string& filepath); // シーンを保存する
	void DrawSceneDialogs(ECS::World* world);                           // シーン操作用のダイアログを描画する
	void RefreshSceneFiles();                       // シーンファイルのリストを更新する
	void QueueSceneSwitch();                        // シーン切り替えの予約を行う

    std::string startupScenePath;
    bool SaveStartupScene(const std::string& filepath); // 起動シーンを記録する

    void StartPlay(ECS::World* world);      // シーン再生の開始
    void StopPlay(ECS::World* world);       // シーン再生の停止

    json CaptureScene(ECS::World* world) const;         // シーンをJSONに保存
    bool RestoreScene(ECS::World* world, const json& sceneJson);    // JSONからシーンを復元

    void DrawHierarchyWindow(ECS::World* world);
    void DrawInspectorWindow(ECS::World* world);
	void DeleteEditorEntity(ECS::World* world, ECS::EntityID id);   // エディタ上でエンティティを削除する

};
