// Copyright EnigmaEngine. All Rights Reserved.

#include "Engine/GameEngine.h"
#include "Engine/EngineDelegates.h"
#include "EngineLoop.h"
#include "EngineModule.h"
#include "CoreGlobals.h"
#include "Misc/ConfigCacheIni.h"
#include "GenericPlatform/GenericApplication.h"
#include "GenericPlatform/GenericWindowDefinition.h"
#include "GenericPlatform/GenericWindow.h"

#include <cstdio>
#include <string>

namespace Enigma
{

// Static factory -- defaults to null (base FGameInstance fallback)
FGameEngine::GameInstanceFactory FGameEngine::s_GameInstanceFactory = nullptr;

void FGameEngine::RegisterGameInstanceFactory(GameInstanceFactory factory)
{
    s_GameInstanceFactory = std::move(factory);
}

void FGameEngine::Init(FEngineLoop* engineLoop)
{
    FEngine::Init(engineLoop);

    // Create the game instance via factory method
    GameInstance = CreateGameInstance();
    if (GameInstance)
    {
        std::printf("[FGameEngine] GameInstance created\n");
    }
    else
    {
        std::fprintf(stderr,
            "[FGameEngine] WARNING: CreateGameInstance returned null\n");
    }

    // Initialize game instance (moved from Start to match UE ordering)
    if (GameInstance)
    {
        GameInstance->Init();

        // Call SetupInput if an input subsystem is registered.
        // Uses ISubsystem interface to avoid linking against EnhancedInput.
        ISubsystem* inputSubsystem = nullptr;
        SubsystemCollection.ForEachSubsystem([&](ISubsystem* s)
        {
            if (std::string(s->GetName()) == "FInputSubsystem")
            {
                inputSubsystem = s;
            }
        });
        if (inputSubsystem)
        {
            // Cast through void* to avoid #include of InputSubsystem.h.
            // FGameInstance::SetupInput takes FInputSubsystem& - the actual type
            // is guaranteed by the name check above.
            GameInstance->SetupInput(
                *reinterpret_cast<FInputSubsystem*>(inputSubsystem));
        }
    }

    // Create game window from config
    m_gameWindow = CreateGameWindow();

    std::printf("[FGameEngine] Init\n");
}

void FGameEngine::Start()
{
    FEngine::Start();

    std::printf("[FGameEngine] Start\n");
}

void FGameEngine::Tick(float deltaTime)
{
    FEngine::Tick(deltaTime);

    // Drive the game instance frame loop
    if (GameInstance)
    {
        GameInstance->BeginFrame();
        GameInstance->Update(deltaTime);

        // Renderer integration (REQ-9): wrap Render() with BeginFrame/EndFrame.
        // TryGetRendererModule() returns nullptr if no renderer is loaded,
        // maintaining backward compatibility with headless/test environments.
        IRendererModule* renderer = TryGetRendererModule();
        if (renderer)
        {
            renderer->BeginFrame();
        }

        FEngineDelegates::OnPreRender.Broadcast();

        GameInstance->Render();

        FEngineDelegates::OnPostRender.Broadcast();

        if (renderer)
        {
            renderer->EndFrame();
        }

        GameInstance->EndFrame();
    }
}

void FGameEngine::Shutdown()
{
    std::printf("[FGameEngine] Shutdown\n");

    // Destroy game window with delegate notification
    if (m_gameWindow)
    {
        FEngineDelegates::OnGameWindowDestroyed.Broadcast(m_gameWindow);

        FGenericApplication* app = FGenericApplication::GetApplication();
        if (app)
        {
            app->DestroyWindow(m_gameWindow);
        }
        m_gameWindow = nullptr;
        std::printf("[FGameEngine] Game window destroyed\n");
    }

    // Shutdown and release the game instance
    if (GameInstance)
    {
        GameInstance->Shutdown();
        GameInstance.reset();
        std::printf("[FGameEngine] GameInstance destroyed\n");
    }

    FEngine::Shutdown();
}

FGameInstance* FGameEngine::GetGameInstance() const
{
    return GameInstance.get();
}

// [TEST] Hot-reload GameInstance recreation. Remove when Editor exists.
void FGameEngine::RecreateGameInstance()
{
    // Shutdown and destroy old instance.
    if (GameInstance)
    {
        GameInstance->Shutdown();
        GameInstance.reset();
    }

    // Flush deferred tick function removals from the destroyed scene/components.
    // Without this, TickTaskManager holds dangling pointers to destroyed objects.
    SubsystemCollection.Tick(0.0f);

    // Create new instance using current factory (from reloaded DLL).
    GameInstance = CreateGameInstance();
    if (GameInstance)
    {
        GameInstance->Init();

        // Reconnect input subsystem.
        ISubsystem* inputSubsystem = nullptr;
        SubsystemCollection.ForEachSubsystem([&](ISubsystem* s)
        {
            if (std::string(s->GetName()) == "FInputSubsystem")
            {
                inputSubsystem = s;
            }
        });
        if (inputSubsystem)
        {
            GameInstance->SetupInput(
                *reinterpret_cast<FInputSubsystem*>(inputSubsystem));
        }

        std::printf("[FGameEngine] [TEST] GameInstance recreated after hot-reload\n");
    }
}

std::unique_ptr<FGameInstance> FGameEngine::CreateGameInstance()
{
    if (s_GameInstanceFactory)
    {
        return s_GameInstanceFactory();
    }
    return std::make_unique<FGameInstance>();
}

FGenericWindow* FGameEngine::CreateGameWindow()
{
    FWindowDefinition windowDef;

    // Defaults (used when GConfig is null or keys are missing)
    windowDef.Width        = 120;
    windowDef.Height       = 40;
    windowDef.Type         = EWindowType::Console;
    windowDef.bIsResizable = false;

    // Store title string with stable lifetime (member or static).
    std::string storedTitle;

    if (GConfig)
    {
        // Window title from project settings
        if (GConfig->GetString("/Script/EngineSettings.GeneralProjectSettings",
                               "ProjectDisplayedTitle", storedTitle, "Game"))
        {
            windowDef.Title = storedTitle.c_str();
        }

        // Window dimensions from engine settings
        int32_t width = 0, height = 0;
        if (GConfig->GetInt("/Script/Engine.GameEngine", "GameWindowWidth", width, "Engine"))
        {
            if (width > 0) windowDef.Width = width;
        }
        if (GConfig->GetInt("/Script/Engine.GameEngine", "GameWindowHeight", height, "Engine"))
        {
            if (height > 0) windowDef.Height = height;
        }

        // Window type
        std::string windowType;
        if (GConfig->GetString("/Script/Engine.GameEngine", "GameWindowType", windowType, "Engine"))
        {
            if (windowType == "Console")
            {
                windowDef.Type = EWindowType::Console;
            }
            else if (windowType == "Native")
            {
                windowDef.Type = EWindowType::Native;
            }
        }

        // Resizable
        GConfig->GetBool("/Script/EngineSettings.GeneralProjectSettings",
                         "bAllowWindowResize", windowDef.bIsResizable, "Game");

        // MaxFPS
        float maxFps = 0.0f;
        if (GConfig->GetFloat("/Script/Engine.GameEngine", "MaxFPS", maxFps, "Engine"))
        {
            if (maxFps > 0.0f)
            {
                SetMaxFPS(maxFps);
            }
        }
    }

    FGenericApplication* app = FGenericApplication::GetApplication();
    if (!app)
    {
        std::fprintf(stderr, "[FGameEngine] WARNING: No application, cannot create window\n");
        return nullptr;
    }

    FGenericWindow* window = app->MakeWindow(windowDef);
    if (window)
    {
        window->Show();

        // Initialize the renderer with the newly created window.
        // NOTE: In UE, renderer initialization (RHI) and window creation are separate
        // steps connected through a Viewport abstraction layer. Our engine currently
        // couples them here for simplicity. When adding multiple render backends
        // (DX12, Vulkan), this should be refactored into a separate
        // InitializeRenderer(window) method to decouple window from renderer.
        IRendererModule* renderer = TryGetRendererModule();
        if (renderer)
        {
            renderer->Initialize(window);
        }

        FEngineDelegates::OnGameWindowCreated.Broadcast(window);
        std::printf("[FGameEngine] Game window created (\"%s\", %dx%d)\n",
            windowDef.Title, windowDef.Width, windowDef.Height);
    }
    else
    {
        std::fprintf(stderr, "[FGameEngine] WARNING: MakeWindow returned null\n");
    }

    return window;
}

} // namespace Enigma
