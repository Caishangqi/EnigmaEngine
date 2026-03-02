// Copyright EnigmaEngine. All Rights Reserved.

#include "ArcadeGameInstance.h"
#include "Engine/Engine.h"
#include "Modules/ModuleManager.h"
#include "RenderCore/AsciiRendererInterface.h"
#include "RenderCore/AsciiCell.h"
#include "SceneView/SceneView.h"
#include "Math/Color.h"
#include "Logging/LogMacros.h"
#include "Logging/LogCategory.h"
#include <nlohmann/json.hpp>
#include <format>

DEFINE_LOG_CATEGORY_STATIC(LogArcade, Info, All);

// Simple pseudo-random helpers for the lit-cell demo pattern.
namespace
{
    /// Deterministic char from frame count (visible ASCII range 33-126).
    char RandomChar(uint64_t seed)
    {
        // LCG-style hash for variety.
        uint64_t h = seed * 6364136223846793005ULL + 1442695040888963407ULL;
        return static_cast<char>(33 + (h >> 16) % 94);
    }

    /// Deterministic color from frame count.
    Enigma::FColor RandomColor(uint64_t seed)
    {
        uint64_t h = seed * 2862933555777941757ULL + 3037000493ULL;
        uint8_t  r = static_cast<uint8_t>((h >> 0) & 0xFF);
        uint8_t  g = static_cast<uint8_t>((h >> 8) & 0xFF);
        uint8_t  b = static_cast<uint8_t>((h >> 16) & 0xFF);
        // Ensure the color is bright enough to be visible.
        r = static_cast<uint8_t>(128 + r / 2);
        g = static_cast<uint8_t>(128 + g / 2);
        b = static_cast<uint8_t>(128 + b / 2);
        return Enigma::FColor(r, g, b);
    }
} // anonymous namespace

void FArcadeGameInstance::Init()
{
    Enigma::FGameInstance::Init();
    TickCount   = 0;
    m_litColumn = 0;

    // REQ-014: JSON validation -- create and output config using nlohmann/json.
    nlohmann::json config = {{"game", "EnigmaArcade"}, {"version", 1}};
    ENIGMA_LOG(LogArcade, Info, "Config: {}", config.dump());

    ENIGMA_LOG(LogArcade, Info, "Engine Initialized");
}

void FArcadeGameInstance::Update(float deltaTime)
{
    Enigma::FGameInstance::Update(deltaTime);
    ++TickCount;

    // Advance the lit column each frame (wraps around buffer width).
    auto& renderer = Enigma::FModuleManager::Get().GetModuleChecked<Enigma::IAsciiRendererModule>("Renderer");
    int32_t bufferWidth = renderer.GetFrameBufferWidth();
    if (bufferWidth > 0)
    {
        m_litColumn = static_cast<int32_t>(TickCount % static_cast<uint64_t>(bufferWidth));
    }
}

void FArcadeGameInstance::Render()
{
    auto& renderer = Enigma::FModuleManager::Get().GetModuleChecked<Enigma::IAsciiRendererModule>("Renderer");

    int32_t bufW = renderer.GetFrameBufferWidth();
    int32_t bufH = renderer.GetFrameBufferHeight();

    // Light up the current column with random chars/colors.
    // BeginFrame (called by engine) clears the buffer each frame,
    // so cells from previous frames are already off.
    for (int32_t y = 1; y < bufH - 1; ++y)
    {
        uint64_t       seed = TickCount * 1000 + static_cast<uint64_t>(y);
        char           ch   = RandomChar(seed);
        Enigma::FColor fg   = RandomColor(seed);
        Enigma::FColor bg   = RandomColor(seed);
        renderer.DrawCell(m_litColumn, y, 0, Enigma::FAsciiCell{ch, Enigma::FColor::Black, bg});
    }

    // --- Frame counter text overlay (Z=1, on top of lit cells) ---
    std::string frameText = std::format("Frame: {}", TickCount);
    renderer.DrawText(0, 0, 1, frameText.c_str(), Enigma::FColor::Yellow, Enigma::FColor::Green);

    // --- Set up FSceneView with camera following the lit cell ---
    Enigma::FSceneView view;
    view.ProjectionMode = Enigma::ECameraProjectionMode::Orthographic;
    view.ViewportX      = 0;
    view.ViewportY      = 0;
    view.ViewportWidth  = bufW;
    view.ViewportHeight = bufH;
    view.OrthoWidth     = static_cast<float>(bufW);
    view.OrthoHeight    = static_cast<float>(bufH);

    // Camera follows the lit column: center the view on it.
    float cameraX                  = static_cast<float>(m_litColumn) - static_cast<float>(bufW) / 2.0f;
    view.ViewTransform.Translation = Enigma::FVector(cameraX, 0.0f, 0.0f);
    renderer.SetActiveView(view);

    // Diagnostics via ENIGMA_LOG (separate from rendered frame).
    ENIGMA_LOG(LogArcade, Info, "Frame: {}, LitColumn: {}", TickCount, m_litColumn);
}

void FArcadeGameInstance::Shutdown()
{
    ENIGMA_LOG(LogArcade, Info, "Game Loop Ended (total ticks: {})", TickCount);
    Enigma::FGameInstance::Shutdown();
}
