#pragma once
#include <Effekseer/Effekseer.h>
#include <Effekseer/EffekseerRendererDX12.h> 
#include "Vector.h"
#include <map>
#include <string>

class EffekseerManager
{
public:
    static void Init();
    static void Uninit();
    static void Update();
    static void Draw();
    static void ClearCache();

    static Effekseer::Handle Play(const char* name, Vector3 position);
    static void Stop(Effekseer::Handle handle);
    static void SetPosition(Effekseer::Handle handle, Vector3 position);
    static void SetRotation(Effekseer::Handle handle, Vector3 rotation);
    static void SetScale(Effekseer::Handle handle, Vector3 scale);

private:
    static ::Effekseer::ManagerRef manager;

    static Effekseer::RefPtr<EffekseerRenderer::Renderer> renderer;
    static Effekseer::RefPtr<EffekseerRenderer::SingleFrameMemoryPool> memoryPool;
    static Effekseer::RefPtr<EffekseerRenderer::CommandList> efkCmdList;

    static std::map<std::string, ::Effekseer::EffectRef> effects;

    static ::Effekseer::EffectRef LoadEffect(const char* path);
};