// Copyright EnigmaEngine. All Rights Reserved.

#include "GameFramework/GameInstance.h"
#include "GameFramework/Scene.h"
#include "GameFramework/GameObject.h"
#include "Misc/AssertionMacros.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogGameInstance, Info, All);

namespace Enigma
{

void FGameInstance::Init()
{
    ENIGMA_LOG(LogGameInstance, Info, "FGameInstance Init");
}

void FGameInstance::Shutdown()
{
    ENIGMA_LOG(LogGameInstance, Info, "FGameInstance Shutdown");
}

void FGameInstance::BeginFrame()
{
    ++FrameCount;
}

void FGameInstance::Update(float deltaTime)
{
    DeltaTime = deltaTime;
    m_sceneManager.Tick(deltaTime);
}

void FGameInstance::Render()
{
    m_sceneManager.RenderScene();
}

void FGameInstance::EndFrame()
{
    // Default: no-op. Users override for end-of-frame cleanup.
}

FScene* FGameInstance::LoadScene(const std::string& name)
{
    return m_sceneManager.LoadScene(name);
}

FScene* FGameInstance::GetActiveScene() const
{
    return m_sceneManager.GetActiveScene();
}

FGameObject* FGameInstance::CreateGameObject(const std::string& name)
{
    FScene* scene = m_sceneManager.GetActiveScene();
    checkf(scene, "No active scene -- call LoadScene() first");
    return scene->CreateGameObject(name);
}

} // namespace Enigma
