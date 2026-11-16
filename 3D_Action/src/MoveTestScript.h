#pragma once

#include "NativeScript.h"

class MoveTestScript : public NativeScript
{
public:
    void Update() override;

    void SaveSettings(JsonWriteArchive& archive) override;
    void LoadSettings(JsonReadArchive& archive) override;
    void DrawSettings(ImGuiArchive& archive) override;

private:
    float moveSpeed = 50.0f;
};
