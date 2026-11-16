#include "MoveTestScript.h"
#include "Archive.h"
#include "EngineTime.h"
#include "Transform.h"

void MoveTestScript::Update()
{
    if (!HasComponent<Transform>())
    {
        return;
    }

    auto& transform = GetComponent<Transform>();
    transform.position.x += moveSpeed * Time::GetDeltaTime();
}

void MoveTestScript::SaveSettings(JsonWriteArchive& archive)
{
    archive.Property("MoveSpeed", moveSpeed);
}

void MoveTestScript::LoadSettings(JsonReadArchive& archive)
{
    archive.Property("MoveSpeed", moveSpeed);
}

void MoveTestScript::DrawSettings(ImGuiArchive& archive)
{
    archive.Property("MoveSpeed", moveSpeed);
}
