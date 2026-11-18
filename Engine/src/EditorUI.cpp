#include "Engine_pch.h"

#include "EditorUI.h"
#include "GraphicsCore.h"
#include <stdexcept>
#include <algorithm>
#include "Name.h" 

#include <fstream>
#include <utility>
#include <iomanip> // 綺麗に改行してJSONを保存するため
#include "UUID.h"
#include "MeshRenderer.h"
#include "SpriteRenderer.h"
#include "Camera.h"
#include "CameraSystem.h"
#include "SceneManager.h"
#include "ColliderSystem.h"
#include "Script.h"
#include "EngineTime.h"
#include "Input.h"
#include "EffekseerManager.h"
#include "Transform.h"
#include <filesystem>
#include <cctype>


namespace
{
    constexpr const char* EDITOR_SETTINGS_PATH = "EditorSettings.json";     // エディタの設定ファイルの名前

    std::string PathToUtf8(const std::filesystem::path& path)
    {   
        const auto text = path.generic_u8string();
        return std::string(text.begin(), text.end());
    }
}

void EditorUI::Initialize(HWND hwnd) {
    editorState = EditorState::Edit;
    auto& sceneManager = SceneManager::GetInstance();
    sceneManager.SetSceneLoadEnabled(false);
    observedSceneRevision = sceneManager.GetSceneRevision();
    pendingDeleteEntity = ECS::INVALID_ENTITY_ID;
    pendingRemoveEntity = ECS::INVALID_ENTITY_ID;
    pendingAction = EditorAction::None;
    editSceneSnapshot = json();
    playbackError.clear();
    startupScenePath.clear();
    currentScenePath.clear();
    sceneError.clear();
    sceneFiles.clear();
    sceneName[0] = '\0';
    sceneDialog = SceneDialog::None;
    pendingSceneOperation = SceneOperation::None;
    requestedSceneOperation = SceneOperation::None;
    pendingScenePath.clear();
    requestedScenePath.clear();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;       // ドッキング機能を有効化
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;   // キーボード有効化

    ImGui::StyleColorsDark();   // 色

    auto& gfx = GraphicsCore::Get();

    bool is = false;
    bool is32 = false;

    // ImGui用のSRVディスクリプタヒープを渡す
    handle = gfx.GetSrvHeapManager().Allocate();

    ImGui_ImplDX12_InitInfo init_info = {};
    init_info.Device = gfx.GetDevice();
    init_info.NumFramesInFlight = 2; // GraphicsCore::frameCount と一致させる
    init_info.RTVFormat = gfx.GetBackBufferFormat();
    init_info.CommandQueue = gfx.GetCommandQueue(); // グラフィックスコアのコマンドキューを共有
    init_info.SrvDescriptorHeap = gfx.GetSrvHeapManager().GetHeap();
    init_info.LegacySingleSrvCpuDescriptor = handle.CPUHandle; // 古いAPIとの互換性のためにこれらを使用
    init_info.LegacySingleSrvGpuDescriptor = handle.GPUHandle;

    is32 = ImGui_ImplWin32_Init(hwnd);
    is = ImGui_ImplDX12_Init(&init_info);

    ImGui::GetIO().Fonts->Build();
    if (!ImGui_ImplDX12_CreateDeviceObjects()) {
        throw std::runtime_error("ImGui");
    }
}

void EditorUI::Shutdown()
{
    // 終了処理
    if (handle.IsValid())
        GraphicsCore::Get().GetSrvHeapManager().Free(handle);
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void EditorUI::BeginUI()
{
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    // 全体をドッキングスペースにする
    //ImGui::DockSpaceOverViewport();
    ImGui::DockSpaceOverViewport(0,nullptr, ImGuiDockNodeFlags_PassthruCentralNode);
}

void EditorUI::RenderUI(ECS::World* world)
{
    auto& sceneManager = SceneManager::GetInstance();
    if (observedSceneRevision != sceneManager.GetSceneRevision())   // 
    {
        observedSceneRevision = sceneManager.GetSceneRevision();
        selectedEntityID = ECS::INVALID_ENTITY_ID;
        pendingDeleteEntity = ECS::INVALID_ENTITY_ID;
        pendingRemoveEntity = ECS::INVALID_ENTITY_ID;
        playbackError = sceneManager.GetSceneLoadError();
        if (!playbackError.empty() && editorState == EditorState::Playing)
        {
            editorState = EditorState::Paused;
            sceneManager.SetSceneLoadEnabled(false);
            SetMouseLock(false);
        }
    }
    // エディタのメインメニューバーを描画
    if (ImGui::BeginMainMenuBar()) {

		// ファイルメニュー
        if (ImGui::BeginMenu("File")) {
            const bool canEditScene = editorState == EditorState::Edit;

            const bool canManageScene = canEditScene && sceneDialog == SceneDialog::None &&
                pendingSceneOperation == SceneOperation::None;

            if (ImGui::MenuItem("New Scene", nullptr, false, canManageScene))   // 新しいシーンを作る
            {
                requestedSceneOperation = SceneOperation::New;
                sceneDialog = SceneDialog::ConfirmSwitch;
                sceneError.clear();
            }
			if (ImGui::MenuItem("Open Scene", nullptr, false, canManageScene))  // シーンを開く
            {
                sceneError.clear();
                RefreshSceneFiles();
                sceneDialog = SceneDialog::Open;
            }
			if (ImGui::MenuItem("Save Scene", nullptr, false, canManageScene))  // シーンを保存する
            {
                sceneError.clear();
				if (currentScenePath.empty())   // シーンのパスが空の場合は別名で保存する
                {
                    sceneName[0] = '\0';
                    sceneDialog = SceneDialog::SaveAs;
                }
                else
                {
                    pendingScenePath = currentScenePath;
                    pendingSceneOperation = SceneOperation::Save;
                }
            }
			if (ImGui::MenuItem("Save Scene As", nullptr, false, canManageScene))   // シーンを別名で保存する
            {
                sceneError.clear();
                sceneName[0] = '\0';
                sceneDialog = SceneDialog::SaveAs;
            }
            ImGui::EndMenu();
        }
        ImGui::Separator();

		// 再生・停止ボタン
		switch (editorState)    // エディタの状態に応じてボタンを切り替える
        {
        case EditorState::Edit:
            ImGui::BeginDisabled(sceneDialog != SceneDialog::None ||
                pendingSceneOperation != SceneOperation::None);
            if (ImGui::Button("Play"))
            {
                pendingAction = EditorAction::Play;
            }
            ImGui::EndDisabled();
            break;

        case EditorState::Playing:
            if (ImGui::Button("Pause"))
            {
                editorState = EditorState::Paused;
                sceneManager.SetSceneLoadEnabled(false);
            }
            break;

        case EditorState::Paused:
            if (ImGui::Button("Resume"))
            {
                editorState = EditorState::Playing;
                sceneManager.SetSceneLoadEnabled(true);
            }
            break;
        }

        if (editorState != EditorState::Edit)
        {
            ImGui::SameLine();

            if (ImGui::Button("Stop"))
            {
                pendingAction = EditorAction::Stop;
            }
        }

        ImGui::SameLine();

		// エディタの状態を表示
        const char* stateName = "Edit";

        if (editorState == EditorState::Playing)
        {
            stateName = "Playing";
        }
        else if (editorState == EditorState::Paused)
        {
            stateName = "Paused";
        }

        ImGui::TextUnformatted(stateName);
        ImGui::SameLine();

		// 現在のシーンの名前を表示する
        const std::string& displayScenePath =
            editorState != EditorState::Edit && !sceneManager.GetLoadedScenePath().empty() ?
            sceneManager.GetLoadedScenePath() : currentScenePath;
        const std::string sceneLabel = displayScenePath.empty() ? "Untitled" :
            PathToUtf8(std::filesystem::u8path(displayScenePath).filename());
        ImGui::Text("Scene: %s", sceneLabel.c_str());

		// エラーがあれば表示する
        if (!sceneError.empty())
        {
            ImGui::SameLine();
            ImGui::TextUnformatted(sceneError.c_str());
        }

		// エラーがあれば表示する
        if (!playbackError.empty())
        {
            ImGui::SameLine();
            ImGui::TextUnformatted(playbackError.c_str());
        }
        ImGui::EndMainMenuBar();
    }

    ImGui::Text("DEBUG: Current Selected ID = %lld", (long long)selectedEntityID);

	DrawHierarchyWindow(world); // エンティティの階層ウィンドウを描画
	DrawInspectorWindow(world); // 選択中のエンティティのインスペクタウィンドウを描画

	DrawSceneDialogs(world);    // シーンのダイアログを描画

    // 描画データ生成
    ImGui::Render();

    auto* commandList = GraphicsCore::Get().GetCommandList();
    //auto handle = GraphicsCore::Get().GetRtvHandle();
    //commandList->OMSetRenderTargets(1, &handle, FALSE, nullptr);

    // ImGuiの描画コマンドを積む前に、専用のディスクリプタヒープをセットする
    ID3D12DescriptorHeap* heaps[] = { GraphicsCore::Get().GetSrvHeapManager().GetHeap()};
    commandList->SetDescriptorHeaps(1, heaps);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
}

void EditorUI::Update(ECS::World* world)
{
    const SceneOperation sceneOperation = pendingSceneOperation;
    const std::string scenePath = pendingScenePath;
    pendingSceneOperation = SceneOperation::None;

    if (sceneOperation != SceneOperation::None)
    {
		pendingScenePath.clear();   // シーン操作がある場合はパスをクリアする
    }

    if (sceneOperation != SceneOperation::None)
    {
		// シーン操作がある場合は、削除・コンポーネント削除・アクションの予約をクリアする
        pendingDeleteEntity = ECS::INVALID_ENTITY_ID;
        pendingRemoveEntity = ECS::INVALID_ENTITY_ID;
        pendingAction = EditorAction::None;

        if (editorState == EditorState::Edit)
        {
            switch (sceneOperation)
            {
            case SceneOperation::New:
				NewScene(world);        // 新しいシーンを作る
                break;

            case SceneOperation::Open:
				OpenScene(world, scenePath);    // シーンを開く
                break;

            case SceneOperation::Save:
                if (SaveSceneFile(world, scenePath))
                {
					QueueSceneSwitch();     // 保存が成功した場合はシーン切り替えを予約する
                }
                else
                {
                    requestedSceneOperation = SceneOperation::None;
                    requestedScenePath.clear();
                }
                break;

            case SceneOperation::None:
                break;
            }
        }
    }
    else if (pendingDeleteEntity != ECS::INVALID_ENTITY_ID)
    {
        const ECS::EntityID id = pendingDeleteEntity;
        pendingDeleteEntity = ECS::INVALID_ENTITY_ID;
        pendingRemoveEntity = ECS::INVALID_ENTITY_ID;

        if (editorState == EditorState::Edit)
        {
			DeleteEditorEntity(world, id);  // エディタ上でエンティティを削除する
        }
    }

	// コンポーネント削除の予約があれば実行する
    const ECS::EntityID removeEntityId = pendingRemoveEntity;
    const ECS::ComponentTypeID removeComponentTypeId = pendingRemoveComponentTypeId;
    pendingRemoveEntity = ECS::INVALID_ENTITY_ID;

    if (editorState == EditorState::Edit && removeEntityId != ECS::INVALID_ENTITY_ID)
    {
        RemoveEditorComponent(world, removeEntityId, removeComponentTypeId);
    }

    const EditorAction action = pendingAction;
    pendingAction = EditorAction::None;

	// 再生・停止の予約アクションがあれば実行する
    switch (action)
    {
    case EditorAction::Play:
        StartPlay(world);
        break;

    case EditorAction::Stop:
        StopPlay(world);
        break;

    case EditorAction::None:
        break;
    }
}

void EditorUI::StartPlay(ECS::World* world)
{
    if (editorState != EditorState::Edit)
    {
        return;
    }

	playbackError.clear();  // エラーをクリア

    bool canStartPlay = true;

    world->ForEachComponent<Script>(
        [&](ECS::EntityID id, Script& script)
        {
			if (script.HasUnavailableType())    // 型名が存在しない場合は再生できない
            {
                canStartPlay = false;
            }
        });

    if (!canStartPlay)
    {
        playbackError = "Cannot play: a script is unavailable.";    // エラー登録
        return;
    }

	editSceneSnapshot = CaptureScene(world);    // 現在のシーンをメモリに保存
    editTimeScale = Time::GetTimeScale();

    auto& sceneManager = SceneManager::GetInstance();
    sceneManager.ResetSceneLoadState();
    sceneManager.SetSceneLoadEnabled(true);
    observedSceneRevision = sceneManager.GetSceneRevision();
    editorState = EditorState::Playing;
}

void EditorUI::StopPlay(ECS::World* world)
{
    if (editorState == EditorState::Edit)
    {
        return;
    }

    editorState = EditorState::Paused;
    SetMouseLock(false);

    // 復元中のOnDestroyからも新しい遷移を予約させない
    auto& sceneManager = SceneManager::GetInstance();
    sceneManager.SetSceneLoadEnabled(false);
    sceneManager.ResetSceneLoadState();

    if (!RestoreScene(world, editSceneSnapshot))
    {
		playbackError = "Failed to restore the edit scene.";    // エラー登録
        return;
    }

	EffekseerManager::StopAll();        // 再生中のエフェクトをすべて停止
	Time::SetTimeScale(editTimeScale);  // 時間のスケールを元に戻す

	editSceneSnapshot = json(); // メモリに保存していたシーンデータをクリア
    playbackError.clear();
    editorState = EditorState::Edit;
}

json EditorUI::CaptureScene(ECS::World* world) const
{
    return SceneSerializer::Capture(world);
}

void EditorUI::SaveScene(ECS::World* world, const std::string& filepath)
{
    SaveSceneFile(world, filepath);
}

void EditorUI::LoadScene(ECS::World* world, const std::string& filepath)
{
    OpenScene(world, filepath);
}

void EditorUI::LoadStartupScene(ECS::World* world)
{
    if (editorState != EditorState::Edit)
    {
        return;
    }

    startupScenePath.clear();
    std::string startupError;

    try
    {
        std::ifstream file(std::filesystem::u8path(EDITOR_SETTINGS_PATH), std::ios::binary);    // 読み込む

        if (!file)
        {
            throw std::runtime_error("Cannot open EditorSettings.json.");   // 読み込めなかったらエラー
        }

        // jsonの構成を確認する
        const json settings = json::parse(file, nullptr, false);
        if (!settings.is_object() ||
            !settings.contains("StartupScene") ||
            !settings["StartupScene"].is_string())
        {
            throw std::runtime_error("Invalid StartupScene setting.");
        }

        const std::string filepath = settings["StartupScene"].get<std::string>();   // パスを読み込む
        if (filepath.empty())
        {
            throw std::runtime_error("StartupScene is empty."); // 読み込めなかったらエラー
        }

        startupScenePath = filepath;
        if (OpenScene(world, filepath)) // シーンを開く
        {
            return;
        }

        startupError = sceneError;
    }
    catch (const std::exception& error)
    {
        startupError = std::string("Startup scene failed: ") + error.what();
    }

    // 起動設定を読めなかった場合はカメラだけのシーンを用意する
    NewScene(world);
    sceneError = startupError;
}

bool EditorUI::SaveStartupScene(const std::string& filepath)
{
    // 設定ファイルと一時ファイルのパスを用意する
    const auto settingsPath = std::filesystem::u8path(EDITOR_SETTINGS_PATH);
    auto temporaryPath = settingsPath;
    temporaryPath += L".tmp";
    bool hasTemporaryFile = false;

    try
    {
        if (std::filesystem::exists(temporaryPath))
        {
            throw std::runtime_error("A temporary settings file already exists.");  // 一時ファイルが既にあったらエラー
        }

        const json settings = { { "StartupScene", filepath } }; // 設定作る

        std::ofstream file(temporaryPath, std::ios::binary);
        hasTemporaryFile = file.is_open();

        if (!hasTemporaryFile)  // 一時ファイル無かったらエラー
        {
            throw std::runtime_error("Cannot create the settings file.");
        }

        // 書き込む
        file << settings.dump(4);
        file.close();

        // 書き込み成功したら一時ファイルを置き換える
        if (!file || !MoveFileExW(temporaryPath.c_str(), settingsPath.c_str(),
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        {
            throw std::runtime_error("Cannot finish saving the settings.");
        }

        hasTemporaryFile = false;
        startupScenePath = filepath;
        return true;
    }
    catch (const std::exception& error)
    {
        if (hasTemporaryFile)
        {
            std::error_code cleanupError;
            std::filesystem::remove(temporaryPath, cleanupError);
        }

        // シーン本体の保存成功と、起動設定の保存失敗を区別して表示する
        sceneError = std::string("Scene saved, but startup setting failed: ") + error.what();
        return false;
    }
}

bool EditorUI::SaveSceneFile(ECS::World* world, const std::string& filepath)
{
    if (editorState != EditorState::Edit)
    {
        return false; // 編集中以外は保存しない
    }

    if (!SceneSerializer::Save(world, filepath, sceneError))
    {
        return false;
    }

    currentScenePath = filepath;
    // シーン保存成功時だけ、次回起動時に開くシーンを更新する
    SaveStartupScene(filepath);
    return true;
}

bool EditorUI::OpenScene(ECS::World* world, const std::string& filepath)
{
    if (editorState != EditorState::Edit)
    {
        return false; // 編集中以外はシーンを開かない
    }

    auto* scene = SceneManager::GetInstance().GetCurrentScene();
    CommandBuffer* commandBuffer = scene && scene->GetWorld() == world ?
        scene->GetCommandBuffer() : nullptr;

    // 読み込みで古いエンティティ参照を残さない
    selectedEntityID = ECS::INVALID_ENTITY_ID;
    pendingDeleteEntity = ECS::INVALID_ENTITY_ID;
    pendingRemoveEntity = ECS::INVALID_ENTITY_ID;
    if (!SceneSerializer::Load(world, filepath, commandBuffer, sceneError))
    {
        return false;
    }

    EffekseerManager::StopAll();
    currentScenePath = filepath;
    return true;
}

void EditorUI::NewScene(ECS::World* world)
{
    // 空のシーンを復元して今あるシーンを消す
    const json emptyScene = { {"Entities", json::array()}, {"MainCameraUUID", uint64_t(0)} };
    if (!RestoreScene(world, emptyScene))
    {
        sceneError = "Cannot create a new scene.";
        return;
    }

    EffekseerManager::StopAll(); // 再生中のエフェクトをすべて停止

    // 新しいシーンのメインカメラを作成する
    const ECS::EntityID cameraId = world->CreateEntity();
    world->AddComponent(cameraId, UUIDComponent{});
    world->AddComponent(cameraId, Name{ "Main Camera" });
    Transform transform;
    transform.position = { 0.0f, 3.0f, -6.0f };
    transform.rotation = Quaternion::FromRotation(15.0f, 0.0f, 0.0f);
    world->AddComponent(cameraId, transform);
    world->AddComponent(cameraId, Camera{});

    // 作成したカメラをメインカメラと選択中のエンティティに設定する
    CameraSystem::SetCamera(cameraId);
    selectedEntityID = cameraId;

    // 新しいシーンには保存先がないためパスをクリアする
    currentScenePath.clear();
    sceneError.clear();
}

void EditorUI::RefreshSceneFiles()
{
    sceneFiles.clear(); // 前回取得したシーンファイルの一覧をクリアする
    try
    {
        // シーンの保存先がない場合は何もしない
        const std::filesystem::path directory = "Assets/scene";
        if (!std::filesystem::exists(directory))
        {
            return;
        }

        // 保存先にあるJsonファイルを一覧に追加する
        for (const auto& entry : std::filesystem::directory_iterator(directory))
        {
            // 拡張子を小文字にそろえて確認する
            std::string extension = PathToUtf8(entry.path().extension());
            std::transform(extension.begin(), extension.end(), extension.begin(),
                [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
            if (entry.is_regular_file() && extension == ".json")
            {
                sceneFiles.push_back(PathToUtf8(entry.path()));
            }
        }
        std::sort(sceneFiles.begin(), sceneFiles.end()); // パス順に並べる
    }
    catch (const std::exception& error)
    {
        sceneError = std::string("Cannot list scenes: ") + error.what(); // エラー登録
    }
}

void EditorUI::QueueSceneSwitch()
{
    // 要求されているシーン操作とパスを実行待ちに登録する
    if (requestedSceneOperation != SceneOperation::None)
    {
        pendingSceneOperation = requestedSceneOperation;
        pendingScenePath = requestedScenePath;
    }
    // 登録した要求をクリアする
    requestedSceneOperation = SceneOperation::None;
    requestedScenePath.clear();
}

void EditorUI::DrawSceneDialogs(ECS::World* world)
{
    // 表示するダイアログがなければ何もしない
    if (sceneDialog == SceneDialog::None)
    {
        return;
    }
    // シーン操作用のモーダルを開く
    if (!ImGui::IsPopupOpen("Scene"))
    {
        ImGui::OpenPopup("Scene");
    }
    if (!ImGui::BeginPopupModal("Scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        return;
    }

    switch (sceneDialog)
    {
    case SceneDialog::ConfirmSwitch:
        // シーンを切り替える前に保存するか確認する
        ImGui::TextUnformatted("Save the current scene before switching?");

        if (ImGui::Button("Save"))
        {
            sceneError.clear();
            if (currentScenePath.empty())
            {
                // 保存先がない場合は名前を付けて保存する
                sceneName[0] = '\0';
                sceneDialog = SceneDialog::SaveAs;
            }
            else
            {
                // 現在の保存先への保存を予約する
                pendingScenePath = currentScenePath;
                pendingSceneOperation = SceneOperation::Save;
                sceneDialog = SceneDialog::None;
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Discard"))
        {
            // 保存せずにシーンの切り替えを予約する
            QueueSceneSwitch();
            sceneDialog = SceneDialog::None;
        }
        break;

    case SceneDialog::Open:
        // シーンファイルの一覧を表示する
        ImGui::TextUnformatted("Open Scene - Assets/scene");

        if (ImGui::Button("Refresh"))
        {
            RefreshSceneFiles();
        }

        ImGui::BeginChild("SceneFiles", ImVec2(400, 220), true);
        if (sceneFiles.empty())
        {
            ImGui::TextUnformatted("No scene files.");
        }
        for (const auto& filepath : sceneFiles)
        {
            const std::string label = PathToUtf8(std::filesystem::u8path(filepath).filename());
            if (ImGui::Selectable(label.c_str(), false, ImGuiSelectableFlags_DontClosePopups))
            {
                // 選んだシーンを記録して切り替え前の保存確認を開く
                requestedSceneOperation = SceneOperation::Open;
                requestedScenePath = filepath;
                sceneDialog = SceneDialog::ConfirmSwitch;
                sceneError.clear();
            }
        }
        ImGui::EndChild();
        break;

    case SceneDialog::SaveAs:
        // 保存するファイル名を入力する
        ImGui::TextUnformatted("Save Scene As - Assets/scene");
        ImGui::InputText("File name", sceneName, sizeof(sceneName));
        if (ImGui::Button("Save"))
        {
            // 使用できない文字やWindowsの予約名が含まれていないか確認する
            std::string name = sceneName;
            std::string upperName = name.substr(0, name.find('.'));
            std::transform(upperName.begin(), upperName.end(), upperName.begin(),[](unsigned char value) { return static_cast<char>(std::toupper(value)); });

            const bool isReservedName = upperName == "CON" || upperName == "PRN" || upperName == "AUX" || upperName == "NUL" ||
                (upperName.size() == 4 && (upperName.substr(0, 3) == "COM" || upperName.substr(0, 3) == "LPT") && upperName[3] >= '1' && upperName[3] <= '9');

            const bool hasControlCharacter = std::any_of(name.begin(), name.end(),[](unsigned char value) { return value < 32; });

            if (name.empty() || name.find_first_of("<>:\"/\\|?*") != std::string::npos ||
                name.back() == '.' || name.back() == ' ' || isReservedName || hasControlCharacter)
            {
                sceneError = "Enter a valid file name without folders.";
            }
            else
            {
                // Jsonの拡張子がなければ追加する
                std::string extension = PathToUtf8(std::filesystem::u8path(name).extension());
                std::transform(extension.begin(), extension.end(), extension.begin(),
                    [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
                if (extension != ".json")
                {
                    name += ".json";
                }
                // 同じ名前のファイルが保存先にあるか確認する
                pendingScenePath = "Assets/scene/" + name;
                std::error_code fileError;
                const bool hasExistingFile = std::filesystem::exists(
                    std::filesystem::u8path(pendingScenePath), fileError);
                if (fileError)
                {
                    sceneError = "Cannot check the destination file.";
                }
                else
                {
                    sceneError.clear();
                    if (hasExistingFile)
                    {
                        // 既存のファイルがあれば上書き確認を開く
                        sceneDialog = SceneDialog::Overwrite;
                    }
                    else
                    {
                        // 新しいファイルへの保存を予約する
                        pendingSceneOperation = SceneOperation::Save;
                        sceneDialog = SceneDialog::None;
                    }
                }
            }
        }
        break;

    case SceneDialog::Overwrite:
        // 同じ名前のファイルを上書きするか確認する
        ImGui::Text("Overwrite %s?", pendingScenePath.c_str());
        if (ImGui::Button("Overwrite"))
        {
            pendingSceneOperation = SceneOperation::Save;
            sceneDialog = SceneDialog::None;
        }

        ImGui::SameLine();
        if (ImGui::Button("Back"))
        {
            sceneDialog = SceneDialog::SaveAs;
        }
        break;

    case SceneDialog::None:
        break;
    }

    if (!sceneError.empty())
    {
        // シーン操作で発生したエラーを表示する
        ImGui::TextUnformatted(sceneError.c_str());
    }
    if (sceneDialog != SceneDialog::None)
    {
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            // 切り替えの要求を取り消してダイアログを閉じる
            requestedSceneOperation = SceneOperation::None;
            requestedScenePath.clear();
            sceneDialog = SceneDialog::None;
            sceneError.clear();
        }
    }
    if (sceneDialog == SceneDialog::None)
    {
        // 操作が終わったらモーダルを閉じる
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

bool EditorUI::RestoreScene(ECS::World* world, const json& sceneJson)
{
    if (!SceneSerializer::IsValid(sceneJson))
    {
        return false;
    }

    selectedEntityID = ECS::INVALID_ENTITY_ID;
    pendingDeleteEntity = ECS::INVALID_ENTITY_ID;
    pendingRemoveEntity = ECS::INVALID_ENTITY_ID;

    auto* scene = SceneManager::GetInstance().GetCurrentScene();
    CommandBuffer* commandBuffer = scene && scene->GetWorld() == world ?
        scene->GetCommandBuffer() : nullptr;
    return SceneSerializer::Restore(world, sceneJson, commandBuffer);
}

void EditorUI::DeleteEditorEntity(ECS::World* world, ECS::EntityID id)
{
    const auto entityIds = world->GetEntityIds();

    if (std::find(entityIds.begin(), entityIds.end(), id) == entityIds.end())
    {
		return; // 既に削除されている場合は何もしない
    }

    GraphicsCore::Get().FlushCommandQueue();

    selectedEntityID = ECS::INVALID_ENTITY_ID;

    if (CameraSystem::GetCamera() == id)
    {
		CameraSystem::SetCamera(ECS::INVALID_ENTITY_ID);    // 削除するエンティティがカメラだった場合はカメラを解除する
    }

	world->DeleteEntity(id);    // エンティティを削除する
    ColliderSystem::Instance().ClearContactHistory();

    SceneSerializer::ResolveLinks(world);
}

void EditorUI::RemoveEditorComponent(ECS::World* world, ECS::EntityID id, ECS::ComponentTypeID componentTypeId)
{
    const auto entityIds = world->GetEntityIds();
    if (std::find(entityIds.begin(), entityIds.end(), id) == entityIds.end())
    {
		return;     // エンティティが既に削除されている場合は何もしない
    }

    const auto entry = std::find_if(componentRegistry.begin(), componentRegistry.end(),[componentTypeId](const ComponentMetaData& info)
        {
            return info.componentTypeId == componentTypeId;
        });

    if (entry == componentRegistry.end() || !entry->canRemove || !entry->hasComponent(world, id))
    {
		return; // コンポーネントが登録されていない、削除できない、またはエンティティがそのコンポーネントを持っていない場合は何もしない
    }

    GraphicsCore::Get().FlushCommandQueue();

	if (componentTypeId == ECS::ComponentRegistry::GetID<Script>()) // Scriptコンポーネントを削除する場合は、スクリプトのDestroy()を呼ぶ
    {
        auto script = world->GetComponent<Script>(id).script;
        if (script)
        {
            script->Destroy();
        }
    }

    // 終了処理でエンティティが削除されていた場合は処理を終える
    const auto remainingEntityIds = world->GetEntityIds();
    if (std::find(remainingEntityIds.begin(), remainingEntityIds.end(), id) == remainingEntityIds.end())
    {
        selectedEntityID = ECS::INVALID_ENTITY_ID;
        return;
    }

    if (componentTypeId == ECS::ComponentRegistry::GetID<Camera>() && CameraSystem::GetCamera() == id)
    {
		CameraSystem::SetCamera(ECS::INVALID_ENTITY_ID);    // 削除するコンポーネントがカメラで、かつそのエンティティが現在のカメラの場合はカメラを解除する
    }

    entry->removeComponent(world, id);
    ColliderSystem::Instance().ClearContactHistory();
}

void EditorUI::DrawHierarchyWindow(ECS::World* world) {
    ImGui::Begin("Hierarchy");

	// "Create Empty Object" ボタンを押すと新しいエンティティを作成する
    if (ImGui::Button("Create Empty Object")) {
        auto newEntity = world->CreateEntity();
        world->AddComponent(newEntity, UUIDComponent{});
        world->AddComponent(newEntity, Name{ "New Object" });
    }

    ImGui::SameLine();

	// "Delete" ボタンは、エディタが編集モードであり、かつエンティティが選択されている場合のみ有効にする
    const bool canDeleteEntity = editorState == EditorState::Edit && selectedEntityID != ECS::INVALID_ENTITY_ID;

    ImGui::BeginDisabled(!canDeleteEntity);

    if (ImGui::Button("Delete"))
    {
		pendingDeleteEntity = selectedEntityID; // 削除予約フラグを立てる
    }

    ImGui::EndDisabled();

    ImGui::Separator();

    // NameComponentを持っている全エンティティをリスト表示
    world->ForEachComponent<Name>([&](ECS::EntityID id, Name& nameComp) {

        // エンティティIDをImGuiの内部IDとして登録する
        ImGui::PushID(static_cast<int>(id));

        bool isSelected = (selectedEntityID == id);
        if (ImGui::Selectable(nameComp.name.c_str(), isSelected)) {
            selectedEntityID = id;
        }

        // 解除
        ImGui::PopID();

        });
    ImGui::End();
}

void EditorUI::DrawInspectorWindow(ECS::World* world) {
    ImGui::Begin("Inspector");

    if (selectedEntityID != ECS::INVALID_ENTITY_ID) {

        // 登録された全コンポーネントのUIを自動生成
        for (auto& info : componentRegistry) {
            info.drawInspector(world, selectedEntityID);
        }

        ImGui::Separator();

        // コンポーネント追加メニューも自動生成
        if (ImGui::Button("Add Component")) {
            ImGui::OpenPopup("AddComponentPopup");
        }
        if (ImGui::BeginPopup("AddComponentPopup")) {
            for (auto& info : componentRegistry) {
                info.addComponent(world, selectedEntityID);
            }
            ImGui::EndPopup();
        }
    }
    else {
        ImGui::Text("No object selected.");
    }

    ImGui::End();
}
