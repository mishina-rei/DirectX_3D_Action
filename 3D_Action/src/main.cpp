#include <windows.h>
#include <exception>
#include <filesystem>
#include <stdexcept>
#include <vector>

#include "Engine.h"
#include "SceneTest.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    try
    {
#if WITH_EDITOR
        // 編集したシーンを次のゲームビルドへ反映できるよう、制作元のデータを使用する
        std::filesystem::current_path(PROJECT_CONTENT_DIRECTORY);
#else
        // ゲーム本編は起動方法によらず、実行ファイルの隣のデータを使用する
        std::vector<wchar_t> executablePath(32768);
        const DWORD length = GetModuleFileNameW(nullptr, executablePath.data(),
            static_cast<DWORD>(executablePath.size()));
        if (length == 0 || length >= executablePath.size())
        {
            throw std::runtime_error("Cannot locate the executable directory.");
        }
        std::filesystem::current_path(
            std::filesystem::path(executablePath.data()).parent_path());
#endif

        EngineCore::Get().Run<SceneTest>(L"My Custom DX12 Engine", 1920, 1080);
    }
    catch (const std::exception& error)
    {
        // エディタなしでも起動・シーン読み込みの失敗理由を確認できるようにする
        MessageBoxA(nullptr, error.what(), "Game Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    return 0;
}
