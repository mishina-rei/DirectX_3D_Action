#pragma once

#include "NativeScript.h"

class MoveTestScript : public NativeScript
{
public:
    void Update() override;

    void SaveSettings(JsonWriteArchive& archive) override;
    void LoadSettings(JsonReadArchive& archive) override;
#if WITH_EDITOR
    void DrawSettings(ImGuiArchive& archive) override;
#endif

private:
    float moveSpeed = 50.0f;
};
