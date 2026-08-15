#pragma once
#include <windows.h>
#include <string>

#include "SceneManager.h"

class EngineCore
{
public:
    static EngineCore& Get()
    {
        static EngineCore instance;
        return instance;
    }

    // ゲーム起動用関数（テンプレートで最初のシーンを受け取る）
    template <class InitialScene>
    void Run(const std::wstring& title, int width, int height)
    {
        if (Initialize(title, width, height))
        {
            // 最初のシーンを登録
            SceneManager::GetInstance().Init<InitialScene>();

            // メインループ開始
            MainLoop();
        }
        Terminate();
    }

    HWND GetWindowHandle() const { return m_hwnd; }

private:
    EngineCore() = default;
    ~EngineCore() = default;
    EngineCore(const EngineCore&) = delete;
    EngineCore& operator=(const EngineCore&) = delete;

    bool Initialize(const std::wstring& title, int width, int height);
    void MainLoop();
    void Terminate();

    // ウィンドウプロシージャ
    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    HWND m_hwnd = nullptr;
    int m_width = 0;
    int m_height = 0;
};